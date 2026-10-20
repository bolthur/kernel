/**
 * Copyright (C) 2018 - 2026 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <libgen.h>
#include <ext2fs/ext2fs.h>
#include <ff.h>
#include "image.h"
#include "global.h"
#include "mbr.h"
#include "path.h"

FILE* global_sd_image_fd;

/**
 * @brief Function to create empty sdcard image
 * @param path path where to store
 * @return
 */
bool image_create_empty( const char* path ) {
  constexpr uint32_t fat_start = FAT_PARTITION_START;
  constexpr uint32_t fat_size = FAT_PARTITION_SIZE;
  constexpr uint32_t ext_start = EXT_PARTITION_START;
  constexpr uint32_t ext_size = EXT_PARTITION_SIZE;

  fprintf( stdout, "Creating basic image at %s.\r\n", path );

  // progress output
  fprintf(
    stdout, "[1/4] Preparing raw disk image skeleton (%u MB)\r\n",
    ( TOTAL_IMAGE_SIZE * 512 ) / 1024 / 1024 );
  // open file
  global_sd_image_fd = fopen( path, "wb+" );
  if ( ! global_sd_image_fd ) {
    return false;
  }
  // truncate file to filesize
  ftruncate( fileno( global_sd_image_fd ), ( off_t )TOTAL_IMAGE_SIZE * 512 );

  // construct mbr
  fprintf( stdout, "[2/4] Constructing master boot record...\r\n" );
  mbr_t mbr = { 0 };
  mbr.signature = 0xAA55;
  // fat partition
  mbr.partition[ 0 ].boot_flag = 0x80;
  mbr.partition[ 0 ].partition_type = 0x0C;
  mbr.partition[ 0 ].start_lba = fat_start;
  mbr.partition[ 0 ].total_sectors = fat_size;
  // ext partition
  mbr.partition[ 1 ].boot_flag = 0x80;
  mbr.partition[ 1 ].partition_type = 0x83;
  mbr.partition[ 1 ].start_lba = ext_start;
  mbr.partition[ 1 ].total_sectors = ext_size;
  // set to beginning
  fseeko( global_sd_image_fd, 0, SEEK_SET );
  fwrite( &mbr, sizeof( mbr_t ), 1, global_sd_image_fd );
  fflush( global_sd_image_fd );

  fseeko( global_sd_image_fd, 0, SEEK_SET );
  mbr_t mbr2 = { 0 };
  fread( &mbr2, sizeof( mbr_t ), 1, global_sd_image_fd );
  fprintf( stdout, "start of fat = %"PRIu32", total = %"PRIu32"\r\n", mbr2.partition[ 0 ].start_lba, mbr2.partition[ 0 ].total_sectors );

  // create fat partition in image
  fprintf( stdout, "[3/4] Structuring FAT32 layout inside Partition 1 via FatFS...\r\n" );
  const MKFS_PARM fat_opt = { .fmt = FM_FAT32, .n_fat = 2, .align = 0, .n_root = 0, .au_size = 2048, };
  BYTE work_buffer[ FF_MAX_SS ];
  // format partition space using FatFS
  const FRESULT result = f_mkfs( "0:", &fat_opt, work_buffer, sizeof( work_buffer ) ) ;
  if ( FR_OK != result ) {
    fprintf( stderr, "Could not create FATFS structure: %d\r\n", result );
    fclose( global_sd_image_fd );
    return false;
  }

  // create ext partition in image
  fprintf( stdout, "[4/4] Structuring EXT2 layout inside Partition 2 via libext2fs...\r\n" );
  ext2_filsys ext2_fs;
  struct ext2_super_block sb_param = { 0 };
  // assign total block volumes available based on 1024-byte filesystem structural sizing
  ext2fs_blocks_count_set( &sb_param, ( ( off_t )ext_size * 512 ) / 4096 );
  sb_param.s_log_block_size = 2;
  // set inode size
  sb_param.s_inode_size = 256;
  // set feature flags
  ext2fs_set_feature_filetype( &sb_param );
  ext2fs_set_feature_sparse_super( &sb_param );
  ext2fs_set_feature_large_file( &sb_param );
  // setup system
  if ( ext2fs_initialize( "root", EXT2_FLAG_RW, &sb_param, &custom_io_manager, &ext2_fs ) ) {
    fprintf( stderr, "libext2fs initializing sequence failed!\r\n" );
    fclose( global_sd_image_fd );
    return false;
  }
  // set the OS Creator and mark it as a clean ext2 filesystem
  ext2_fs->super->s_creator_os = EXT2_OS_LINUX;
  // build the mandatory initial filesystem tables (Inodes, Blocks, Root directory)
  if ( ext2fs_allocate_tables( ext2_fs ) ) {
    fprintf( stderr, "Error while allocating baseline structures\r\n" );
    ext2fs_close_free( &ext2_fs );
    return false;
  }

  struct ext2_inode inode;
  // Create the directory layout on the system's designated root inode
  errcode_t ext_result = ext2fs_mkdir( ext2_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, nullptr );
  if ( ext_result ) {
    fprintf( stderr, "Error while creating root directory\r\n" );
    ext2fs_close_free( &ext2_fs );
    return false;
  }
  // set root ownership permissions (Typically UID 0, GID 0)
  ext_result = ext2fs_read_inode( ext2_fs, EXT2_ROOT_INO, &inode );
  if ( ext_result ) {
    fprintf( stderr, "Error while reading inode structures\r\n" );
    ext2fs_close_free( &ext2_fs );
    return false;
  }
  inode.i_uid = 0;
  inode.i_gid = 0;
  inode.i_mode = LINUX_S_IFDIR | 0755;
  // write back the completed inode to the filesystem
  ext_result = ext2fs_write_new_inode( ext2_fs, EXT2_ROOT_INO, &inode );
  if ( ext_result ) {
    fprintf( stderr, "Error while writing inode structures\r\n" );
    ext2fs_close_free( &ext2_fs );
    return false;
  }
  // write out superblocks, groups descriptors, and metadata templates
  if ( ext2fs_close_free( &ext2_fs ) ) {
    fprintf( stderr, "Unable to flush ext2fs changes\r\n" );
    fclose( global_sd_image_fd );
    return false;
  }
  fclose( global_sd_image_fd );
  // completed message
  fprintf( stdout, "Completed! Created basic image at %s.\r\n", path );
  return true;
}

/**
 * @brief Copy file to boot image
 * @param path
 * @param destination
 * @return
 */
bool image_copy_file_to_boot( const char* path, const char* destination ) {
  // open file
  global_sd_image_fd = fopen( image_path, "rb+" );
  if ( ! global_sd_image_fd ) {
    return false;
  }
  // mount volume
  FATFS fs;
  FRESULT result = f_mount( &fs, "0:", 1 ) ;
  if ( FR_OK != result ) {
    fprintf( stderr, "Could not create FATFS structure: %d\r\n", result );
    fclose( global_sd_image_fd );
    return false;
  }
  // build destination
  char path_buf[ 1024 ] = "0:\0";
  // duplicate destination
  char* dup_dest = strdup( destination );
  if ( ! dup_dest ) {
    fprintf( stderr, "Could not create duplicate of destination\r\n" );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    return false;
  }
  // separate by directory
  char* dir = dirname( dup_dest );
  const char* token = strtok( dir, PATH_SEP_STR );
  while ( token ) {
    // Add separator when we're not at the beginning
    if ( strlen( path_buf ) ) {
      strcat( path_buf, PATH_SEP_STR );
    }
    // concat current token
    strcat( path_buf, token );
    // check for existence
    FILINFO fno;
    FRESULT fr = f_stat( path_buf, &fno );
    if ( FR_NO_FILE == fr || FR_NO_PATH == fr ) {
      fr = f_mkdir( path_buf );
      if ( FR_OK != fr ) {
        fprintf( stderr, "Could not create duplicate of destination\r\n" );
        f_unmount( "0:" );
        fclose( global_sd_image_fd );
        free( dup_dest );
        return false;
      }
    }
    // go to next token
    token = strtok( nullptr, PATH_SEP_STR );
  }
  // free again
  free( dup_dest );
  // clear path buf again and populate full path
  memset( path_buf, 0, sizeof( path_buf ) );
  if ( PATH_SEP == destination[ 0 ] ) {
    sprintf( path_buf, "0:%s", destination );
  } else {
    sprintf( path_buf, "0:%s%s", PATH_SEP_STR, destination );
  }
  // open file with create
  FIL fil;
  result = f_open( &fil, path_buf, FA_WRITE | FA_CREATE_ALWAYS );
  if ( FR_OK != result ) {
    fprintf( stderr, "Could not create file %s in boot: %d\r\n", path_buf, result );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    return false;
  }
  // open path for reading
  FILE* path_fd = fopen( path, "r" );
  if ( ! path_fd ) {
    fprintf( stderr, "Could not open file %s\r\n", path );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    return false;
  }
  const off_t end = fseeko( path_fd, 0, SEEK_END );
  if ( -1 == end ) {
    fprintf( stderr, "Could not seek end of file %s\r\n", path );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    return false;
  }
  const off_t size = ftello( path_fd );
  if ( -1 == size ) {
    fprintf( stderr, "Could not get size of file %s\r\n", path );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    return false;
  }
  void* buf = malloc( sizeof( char ) * size );
  if ( ! buf ) {
    fprintf( stderr, "Could not allocate memory for file %s\r\n", path );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    return false;
  }
  if ( -1 == fseeko( path_fd, 0, SEEK_SET ) ) {
    fprintf( stderr, "Could not jump back to start\r\n" );
    f_unmount("0:");
    fclose( global_sd_image_fd );
    fclose( path_fd );
    free( buf );
    return false;
  }
  if ( size != fread( buf, sizeof( char ), size, path_fd ) ) {
    fprintf( stderr, "Could not read %s into memory\r\n", path );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    free( buf );
    return false;
  }
  // write file to target
  UINT bw;
  result = f_write( &fil, buf, sizeof( char ) * size, &bw );
  if ( FR_OK != result || bw != size ) {
    fprintf( stderr, "Could not write %s to file %s\r\n", path, path_buf );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    free( buf );
    return false;
  }
  // close file again
  result = f_close( &fil );
  if ( FR_OK != result ) {
    fprintf( stderr, "Could not close file %s\r\n", path );
    f_unmount( "0:" );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    free( buf );
    return false;
  }
  f_unmount( "0:" );
  fclose( global_sd_image_fd );
  fclose( path_fd );
  free( buf );
  return true;
}

/**
 * Helper to ensure that a single directory exists
 * @param fs file system
 * @param parent parent ino
 * @param name directory name
 * @param child child ino output
 * @return
 */
static errcode_t extfs_ensure_single_directory(
  ext2_filsys fs,
  const ext2_ino_t parent,
  const char* name,
  ext2_ino_t* child
) {
  // check for existence
  errcode_t result = ext2fs_lookup( fs, parent, name, strlen( name ), nullptr, child );
  // 0 => exists already
  if ( 0 == result ) {
    return 0;
  }
  // handle not found
  if ( EXT2_ET_FILE_NOT_FOUND == result ) {
    // create directory
    result = ext2fs_mkdir( fs, parent, 0, name );
    // handle error
    if ( result ) {
      fprintf( stderr, "Unable to create directory %s: %ld\r\n", name, result );
      return result;
    }
    // check for existence
    result = ext2fs_lookup( fs, parent, name, strlen( name ), nullptr, child );
    // handle error
    if ( result ) {
      fprintf( stderr, "Unable to lookup ino of %s: %ld\r\n", name, result );
      return result;
    }
    // return success
    return 0;
  }
  // else => return error code
  return result;
}

/**
 * Helper to ensure that directory is existing
 * @param fs file system
 * @param path path
 * @param child final output inode
 * @return
 */
static errcode_t extfs_ensure_directory(
  ext2_filsys fs,
  const char* path, ext2_ino_t* child
) {
  char* copy = strdup( path );
  if ( ! copy ) {
    fprintf( stderr, "Could not duplicate path %s\r\n", path );
    return ENOMEM;
  }
  // start with root node
  ext2_ino_t current_ino = EXT2_ROOT_INO;
  *child = current_ino;
  const char* token = strtok( copy, PATH_SEP_STR );
  errcode_t result = 0;
  while ( token ) {
    // handle only if valid string length
    if ( strlen( token ) ) {
      // ensure single directory existence
      result = extfs_ensure_single_directory( fs, current_ino, token, child );
      // handle error
      if ( result ) {
        fprintf( stderr, "Could not ensure directory\r\n" );
        break;
      }
      // set current ino to child
      current_ino = *child;
    }
    // get next
    token = strtok( nullptr, PATH_SEP_STR );
  }
  // free copy again
  free( copy );
  // return result
  return result;
}

/**
 * Helper to check for file exists
 * @param fs
 * @param path
 * @return
 */
static int check_root_file_existence( ext2_filsys fs, const char* path ) {
  char* copy = strdup( path );
  if ( ! copy ) {
    fprintf( stderr, "Could not duplicate path %s\r\n", path );
    return ENOMEM;
  }
  if ( PATH_SEP == copy[0] ) {
    memmove( copy, copy + 1, strlen( copy ) );
  }
  ext2_ino_t out_ino;
  const errcode_t retval = ext2fs_namei( fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &out_ino );
  free( copy );
  if ( 0 == retval ) {
    return 1;
  }
  if ( EXT2_ET_FILE_NOT_FOUND == retval ) {
    return 0;
  }
  return -1;
}

/**
 * Helper to copy file to root
 * @param path
 * @param destination
 * @return
 */
bool image_copy_file_to_root( const char* path, const char* destination ) {
  // open file
  global_sd_image_fd = fopen( image_path, "rb+" );
  if ( ! global_sd_image_fd ) {
    return false;
  }
  // open ext2 fs
  ext2_filsys ext2_fs;
  errcode_t result = ext2fs_open( "root", EXT2_FLAG_RW, 0, 0, &custom_io_manager, &ext2_fs );
  if ( result ) {
    fprintf( stderr, "Unable to open root partition \"%s\": %ld!\r\n", image_path, result );
    fclose( global_sd_image_fd );
    return false;
  }
  // load bitmaps
  result = ext2fs_read_bitmaps( ext2_fs );
  if ( result ) {
    fprintf( stderr, "Error while loading bitmaps: %ld\n", result );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return false;
  }
  // ensure file not exists
  const int lookup = check_root_file_existence( ext2_fs, destination );
  if ( -1 == lookup ) {
    fprintf( stderr, "Could not lookup file: %s\r\n", destination );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return false;
  }
  // handle already existing
  if ( 1 == lookup ) {
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return true;
  }
  // load file into memory
  FILE* path_fd = fopen( path, "r" );
  if ( ! path_fd ) {
    fprintf( stderr, "Could not open file %s\r\n", path );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return false;
  }
  const off_t end = fseeko( path_fd, 0, SEEK_END );
  if ( -1 == end ) {
    fprintf( stderr, "Could not seek end of file %s\r\n", path );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    return false;
  }
  const off_t size = ftello( path_fd );
  if ( -1 == size ) {
    fprintf( stderr, "Could not get file size of %s\r\n", path );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    return false;
  }
  void* buf = malloc( sizeof( char ) * size );
  if ( ! buf ) {
    fprintf( stderr, "Could not allocate memory for file %s\r\n", path );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    return false;
  }
  if ( -1 == fseeko( path_fd, 0, SEEK_SET ) ) {
    fprintf( stderr, "Could not read %s into memory\r\n", path );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    free( buf );
    return false;
  }
  if ( size != fread( buf, sizeof( char ), size, path_fd ) ) {
    fprintf( stderr, "Could not read %s into memory\r\n", path );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    fclose( path_fd );
    free( buf );
    return false;
  }
  fclose( path_fd );
  // duplicate destination
  char* dup_dest = dirname( strdup( destination ) );
  if ( ! dup_dest ) {
    fprintf( stderr, "Could not create duplicate of destination\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // ensure directory existence
  ext2_ino_t target_ino;
  if ( extfs_ensure_directory( ext2_fs, dup_dest, &target_ino ) ) {
    fprintf( stderr, "Could not create %s\r\n", dup_dest );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    free( dup_dest );
    return false;
  }
  // free again
  free( dup_dest );
  // variables
  ext2_file_t file_handle;
  struct ext2_inode inode;
  unsigned int bytes_written = 0;
  // allocate new inode for file
  ext2_ino_t file_ino;
  result = ext2fs_new_inode( ext2_fs, target_ino, LINUX_S_IFREG | 0644, nullptr, &file_ino );
  if ( result ) {
    fprintf( stderr, "Could not create inode for file\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // link the new inode to the parent directory filename
  char* dup = strdup( destination );
  if ( ! dup ) {
    fprintf( stderr, "Could not duplicate destination\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // try to link
  result = ext2fs_link( ext2_fs, target_ino, basename( dup ), file_ino, EXT2_FT_REG_FILE );
  // handle no space
  if ( result == EXT2_ET_DIR_NO_SPACE ) {
    // expand directory
    result = ext2fs_expand_dir( ext2_fs, target_ino );
    // handle error
    if ( result ) {
      fprintf( stderr, "Could not expand directory\r\n" );
      ext2fs_close( ext2_fs );
      fclose( global_sd_image_fd );
      free( buf );
      free( dup );
      return false;
    }
    // try to link again
    result = ext2fs_link( ext2_fs, target_ino, basename( dup ), file_ino, EXT2_FT_REG_FILE );
    if ( result ) {
      fprintf( stderr, "Could not link directory: %ld\r\n", result );
      ext2fs_close( ext2_fs );
      fclose( global_sd_image_fd );
      free( buf );
      free( dup );
      return false;
    }
  } else if ( result ) {
    fprintf( stderr, "Could not link directory: %ld\r\n", result );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    free( dup );
    return false;
  }
  free( dup );
  // update the block bitmap to finalize inode reservation
  ext2fs_inode_alloc_stats2( ext2_fs, file_ino, 1, 0 );
  // clear inode structure and initialize
  memset( &inode, 0, sizeof( inode ) );
  inode.i_mode = LINUX_S_IFREG | 0644;
  inode.i_links_count = 1;
  inode.i_size = 0;
  inode.i_atime = inode.i_ctime = inode.i_mtime = time( nullptr );
  // write structural metadata to the filesystem table
  result = ext2fs_write_new_inode( ext2_fs, file_ino, &inode );
  if ( result ) {
    fprintf( stderr, "Could write inode\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // open the allocated inode
  result = ext2fs_file_open( ext2_fs, file_ino, EXT2_FILE_WRITE, &file_handle );
  if ( result ) {
    fprintf( stderr, "Could not open file for writing\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // write your raw memory buffer into the file
  result = ext2fs_file_write( file_handle, buf, size, &bytes_written );
  if ( result ) {
    fprintf( stderr, "Could open file for writing\r\n" );
    ext2fs_file_close( file_handle );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // synchronize file pointers and finalize block allocation mapping
  result = ext2fs_file_close( file_handle );
  if ( result ) {
    fprintf( stderr, "Could not close file\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( buf );
    return false;
  }
  // mark inode and block bitmaps dirty
  ext2fs_mark_bb_dirty( ext2_fs );
  ext2fs_mark_ib_dirty( ext2_fs );
  ext2fs_mark_super_dirty( ext2_fs );
  // mark filesystem modified so changes serialize properly
  ext2fs_mark_changed( ext2_fs );
  ext2fs_close( ext2_fs );
  fclose( global_sd_image_fd );
  free( buf );
  return true;
}

/**
 * Helper to copy file to root
 * @param target
 * @param destination
 * @return
 */
bool image_link_file_in_root( const char* target, const char* destination ) {
  // open file
  global_sd_image_fd = fopen( image_path, "rb+" );
  if ( ! global_sd_image_fd ) {
    return false;
  }
  // open ext2 fs
  ext2_filsys ext2_fs;
  errcode_t result = ext2fs_open( "root", EXT2_FLAG_RW, 0, 0, &custom_io_manager, &ext2_fs );
  if ( result ) {
    fprintf( stderr, "Unable to open root partition \"%s\": %ld!\r\n", image_path, result );
    fclose( global_sd_image_fd );
    return false;
  }
  // load bitmaps
  result = ext2fs_read_bitmaps( ext2_fs );
  if ( result ) {
    fprintf( stderr, "Error while loading bitmaps: %ld\n", result );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return false;
  }
  // duplicate destination
  char* dup_dest = dirname( strdup( destination ) );
  if ( ! dup_dest ) {
    fprintf( stderr, "Could not create duplicate of destination\r\n" );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return false;
  }
  // ensure directory existence
  ext2_ino_t target_ino;
  if ( extfs_ensure_directory( ext2_fs, dup_dest, &target_ino ) ) {
    fprintf( stderr, "Could not create %s\r\n", dup_dest );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( dup_dest );
    return false;
  }
  // free again
  free( dup_dest );
  char* dup_target = strdup( target );
  if ( ! dup_target ) {
    fprintf( stderr, "Could not create %s\r\n", dup_dest );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    free( dup_dest );
    return false;
  }
  // create symlink
  result = ext2fs_symlink( ext2_fs, target_ino, 0, destination, basename( dup_target ) );
  if ( result ) {
    fprintf( stderr, "Could not create symlink %s to %s: %ld\r\n", basename( dup_target ), destination, result );
    ext2fs_close( ext2_fs );
    fclose( global_sd_image_fd );
    return false;
  }
  free( dup_target );
  // mark filesystem modified so changes serialize properly
  ext2fs_mark_changed( ext2_fs );
  ext2fs_close( ext2_fs );
  fclose( global_sd_image_fd );
  return true;
}
