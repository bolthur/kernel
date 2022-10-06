/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "internal.h"
#include <sys/bolthur.h>
#include <inttypes.h>

/**
 * @fn bool fat_directory_open(fat_fs_t*, const char*, fat_directory_t*)
 * @brief Open a directory
 *
 * @param fs
 * @param path
 * @param dir
 * @return
 */
bool fat_directory_open(
  __unused fat_fs_t* fs,
  __unused const char* path,
  __unused fat_directory_t* dir
) {
  errno = ENOSYS;
  return false;
}

/**
 * @fn bool fat_directory_read(fat_directory_t*)
 * @brief Read a directory content
 *
 * @param dir
 * @return
 */
bool fat_directory_read( __unused fat_directory_t* dir ) {
  errno = ENOSYS;
  return false;
}

/**
 * @fn void fat_directory_close(fat_directory_t*)
 * @brief Close directory
 *
 * @param dir
 */
void fat_directory_close( __unused fat_directory_t* dir ) {
}

/**
 * @fn bool fat_directory_open_root(fat_fs_t*, fat_directory_t*)
 * @brief Internal method to open root directory
 *
 * @param
 * @param
 * @return
 */
bool fat_directory_open_root(
  __unused fat_fs_t* fs,
  __unused fat_directory_t* dir
) {
  errno = ENOSYS;
  return false;
}

/**
 * @fn uint32_t fat_directory_files_per_sector(fat_fs_t*)
 * @brief Helper to get amount of max files per sector
 *
 * @param fs
 * @return
 */
uint32_t fat_directory_files_per_sector( fat_fs_t* fs ) {
  return fs->boot_sector->bytes_per_sector / sizeof( fat_directory_entry_raw_t );
}

/**
 * @fn bool find_directory_get_next_sector(fat_fs_t*, const char*, uint32_t, fat_directory_file_t*)
 * @brief Method to get next sector of a folder
 *
 * @param fs
 * @param path
 * @param sector
 * @param buffer
 * @return
 */
bool fat_find_directory_get_next_sector(
  fat_fs_t* fs,
  __unused const char* path,
  __unused uint32_t sector,
  __unused fat_directory_entry_raw_t* buffer
) {
  if ( FAT_FAT32 != fs->type ) {
    errno = ENOSYS;
    return false;
  }
  // get first root directory sector
  uint32_t first_sector = fs->fat_data.first_data_sector - fs->fat_data.root_dir_sectors;
  //if ( FAT_FAT32 == fs->type || FAT_EXFAT == fs->type ) {
    first_sector = ( (
      ( fs->boot_sector->extended.fat32.root_cluster - 2 )
        * fs->boot_sector->sectors_per_cluster
    ) + fs->fat_data.first_data_sector );
    EARLY_STARTUP_PRINT( "first_sector = %"PRIu32"\r\n", first_sector )
    EARLY_STARTUP_PRINT( "root_cluster = %"PRIu32"\r\n", fs->boot_sector->extended.fat32.root_cluster )
    EARLY_STARTUP_PRINT( "first_data_sector = %"PRIu32"\r\n", fs->fat_data.first_data_sector )
    EARLY_STARTUP_PRINT( "partition_offset = %"PRIu32"\r\n", fs->partition_sector_offset )
    EARLY_STARTUP_PRINT( "root_dir_sectors = %"PRIu32"\r\n", fs->fat_data.root_dir_sectors )
  //}

  char *long_name_buffer = malloc( PATH_MAX );
  if ( ! long_name_buffer ) {
    errno = ENOMEM;
    return false;
  }
  memset( long_name_buffer, 0, PATH_MAX );
  uint8_t* data = NULL;
  uint32_t data_size = 0;

  uint32_t next_cluster = fs->boot_sector->extended.fat32.root_cluster;
  do {
    EARLY_STARTUP_PRINT( "next_cluster = %"PRId32" - %"PRIx32"\r\n", next_cluster, next_cluster );
    uint32_t cluster_size = fs->boot_sector->bytes_per_sector * fs->boot_sector->sectors_per_cluster;
    uint32_t fat_offset = next_cluster * 4;
    uint32_t fat_sector = fs->fat_data.first_fat_sector + ( fat_offset / cluster_size );
    uint32_t entry_offset = fat_offset % cluster_size;
    /*EARLY_STARTUP_PRINT( "cluster_size = %"PRIu32"\r\n", cluster_size )
    EARLY_STARTUP_PRINT( "fat_offset = %"PRIu32"\r\n", fat_offset )
    EARLY_STARTUP_PRINT( "fat_sector = %"PRIu32"\r\n", fat_sector )
    EARLY_STARTUP_PRINT( "entry_offset = %"PRIu32"\r\n", entry_offset )*/
    // determine sector
    first_sector = ( (
      ( next_cluster - 2 ) * fs->boot_sector->sectors_per_cluster
    ) + fs->fat_data.first_data_sector );
    /*EARLY_STARTUP_PRINT( "first_sector = %"PRIu32"\r\n", first_sector )
    EARLY_STARTUP_PRINT( "root_cluster = %"PRIu32"\r\n", fs->boot_sector->extended.fat32.root_cluster )
    EARLY_STARTUP_PRINT( "first_data_sector = %"PRIu32"\r\n", fs->fat_data.first_data_sector )
    EARLY_STARTUP_PRINT( "partition_offset = %"PRIu32"\r\n", fs->partition_sector_offset )
    EARLY_STARTUP_PRINT( "root_dir_sectors = %"PRIu32"\r\n", fs->fat_data.root_dir_sectors )*/

    // get fat cache
    cache_block_t* fat_cache = fs->cache_block_allocate( fs->handle, fat_sector, true );
    if ( ! fat_cache ) {
      free( long_name_buffer );
      if ( data ) {
        free( data );
      }
      errno = ENOMEM;
      return false;
    }
    // get next in cluster chain
    memcpy( &next_cluster, &fat_cache->data[ entry_offset ], sizeof( next_cluster ) );
    // ignore top most 4 bit
    if ( FAT_FAT32 == fs->type ) {
      next_cluster &= 0x0FFFFFFF;
    }
    // free cache block again
    fs->cache_block_release( fat_cache, false );
    // load sector to cache
    cache_block_t* cache = fs->cache_block_allocate( fs->handle, first_sector, true );
    if ( ! cache ) {
      free( long_name_buffer );
      if ( data ) {
        free( data );
      }
      errno = ENOMEM;
      return false;
    }
    // allocate / reallocate buffer and store
    uint32_t offset = data_size;
    data_size += cache->block_size;
    // reallocate data
    uint8_t* data_new = realloc( data, data_size );
    if ( ! data_new ) {
      fs->cache_block_release( cache, false );
      free( long_name_buffer );
      if ( data ) {
        free( data );
      }
      errno = ENOMEM;
      return false;
    }
    data = data_new;
    // copy over
    memcpy( data + offset, cache->data, cache->block_size );
    // free cache block again
    fs->cache_block_release( cache, false );
  } while ( next_cluster < 0x0FFFFFF8 );
  size_t max_entry_count = data_size / sizeof( fat_directory_entry_raw_t );
  // loop through directories and print them
  for ( size_t idx = 0; idx < max_entry_count; idx++ ) {
    fat_directory_entry_raw_t* file = ( fat_directory_entry_raw_t* )(
      &data[ idx * sizeof( *file ) ]
    );
    // handle end reached
    if ( 0 == file->name[ 0 ] ) {
      EARLY_STARTUP_PRINT( "No further files in sector!\r\n" )
      break;
    }
    // handle unused
    if ( 0xE5 == file->name[ 0 ] ) {
      EARLY_STARTUP_PRINT( "Entry is unused!\r\n" )
      continue;
    }
    // handle long filename
    if ( 0x0F == file->attributes ) {
      fat_directory_entry_long_name_raw_t* long_file = ( fat_directory_entry_long_name_raw_t* )(
        &data[ idx * sizeof( *long_file ) ]
      );
      uint8_t order = long_file->order;
      size_t count = 0;
      if ( ( order | 0x40 ) == order ) {
        memset( long_name_buffer, 0, PATH_MAX );
        count = ( size_t )( order - 0x40 );
      }
      // end of string
      memset( long_name_buffer, 0, PATH_MAX );
      for (size_t j = count; j > 0; j--) {
        long_file = ( fat_directory_entry_long_name_raw_t* )(
          &data[ ( idx + j - 1 ) * sizeof( *long_file ) ]
        );
        // fill tmp buffer
        long_name_buffer[ ( count - j ) * 13 +  0 ] = long_file->first_five_two_byte[ 0 ];
        long_name_buffer[ ( count - j ) * 13 +  1 ] = long_file->first_five_two_byte[ 2 ];
        long_name_buffer[ ( count - j ) * 13 +  2 ] = long_file->first_five_two_byte[ 4 ];
        long_name_buffer[ ( count - j ) * 13 +  3 ] = long_file->first_five_two_byte[ 6 ];
        long_name_buffer[ ( count - j ) * 13 +  4 ] = long_file->first_five_two_byte[ 8 ];
        long_name_buffer[ ( count - j ) * 13 +  5 ] = long_file->next_six_two_byte[ 0 ];
        long_name_buffer[ ( count - j ) * 13 +  6 ] = long_file->next_six_two_byte[ 2 ];
        long_name_buffer[ ( count - j ) * 13 +  7 ] = long_file->next_six_two_byte[ 4 ];
        long_name_buffer[ ( count - j ) * 13 +  8 ] = long_file->next_six_two_byte[ 6 ];
        long_name_buffer[ ( count - j ) * 13 +  9 ] = long_file->next_six_two_byte[ 8 ];
        long_name_buffer[ ( count - j ) * 13 + 10 ] = long_file->next_six_two_byte[ 10 ];
        long_name_buffer[ ( count - j ) * 13 + 11 ] = long_file->final_two_byte[ 0 ];
        long_name_buffer[ ( count - j ) * 13 + 12 ] = long_file->final_two_byte[ 2 ];
      }

      for ( size_t j = 0; j < PATH_MAX; j++ ) {
        if ( 0xFF == long_name_buffer[ j ] ) {
          long_name_buffer[ j ] = 0x20; // Replace with spaces
        }
      }
      // update idx to skip long file name stuff completely
      idx += count;
      EARLY_STARTUP_PRINT( "path: %s\r\n", long_name_buffer )
      /// FIXME: READ POTION AND CONTINUE
      continue;
    }

    char display_name[ 13 ];
    memset( display_name, 0, sizeof( char ) * 13 );
    size_t position = 0;
    for ( size_t j = 0; j < 8; j++ ) {
      char c = file->name[ j ];
      // skip spaces
      if ( isspace( c ) ) {
        continue;
      }
      // transform upper case to lower case
      if ( 'A' <= c && 'Z' >= c ) {
        c += 32;
      }
      display_name[ position++ ] = c;
    }
    uint32_t extension_written = 0;
    for ( size_t j = 0; j < 3; j++ ) {
      char c = file->extension[ j ];
      // skip spaces
      if ( isspace( c ) ) {
        continue;
      }
      // transform upper case to lower case
      if ( 'A' <= c && 'Z' >= c ) {
        c += 32;
      }
      // save point where dot shall take place
      if ( ! extension_written ) {
        extension_written = ++position;
      }
      // write extension
      display_name[ position++ ] = c;
    }
    if ( extension_written ) {
      display_name[ extension_written ] = '.';
    }
    // print filename
    EARLY_STARTUP_PRINT( "path: %s\r\n", display_name )
  }
  if ( data ) {
    free( data );
  }
  free( long_name_buffer );
  // load root directory
  return false;
}
