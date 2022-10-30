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
#include "../fat.h"
#include <sys/bolthur.h>
#include <inttypes.h>

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
 * @fn uint32_t fat_directory_root_cluster(fat_fs_t*)
 * @brief Helper to get root cluster start
 *
 * @param fs
 * @return
 */
uint32_t fat_directory_root_cluster( fat_fs_t* fs ) {
  // get first block
  uint32_t root_cluster = fs->boot_sector->extended.fat32.root_cluster;
  // special handling for non fat32 and non exfat
  if ( FAT_FAT32 != fs->type && FAT_EXFAT != fs->type ) {
    root_cluster = fs->boot_sector->reserved_sector_count +
      ( fs->boot_sector->table_count * fs->fat_data.fat_size );
  }
  // return root cluster
  return root_cluster;
}

/**
 * @fn bool fat_directory_open_root_directory(fat_fs_t*, fat_directory_t*)
 * @brief Helper to open root directory
 *
 * @param fs
 * @param dir
 * @return
 */
bool fat_directory_open_root_directory(
  fat_fs_t* fs,
  fat_directory_t* dir
) {
  if ( fs->type != FAT_FAT32 ) {
    errno = EINVAL;
    return false;
  }
  fat_file_t tmp_file;
  fat_directory_entry_t tmp_entry = {
    .size = 0,
    .first_cluster = fs->boot_sector->extended.fat32.root_cluster,
  };
  // try to open file
  if ( ! fat_file_open_by_directory( fs, &tmp_entry, &tmp_file ) ) {
    return false;
  }
  // allocate buffer
  fat_directory_entry_raw_t* buffer = malloc( tmp_file.size );
  if ( ! buffer ) {
    fat_file_close( &tmp_file );
    errno = ENOMEM;
    return false;
  }
  // try to read content
  if ( ! fat_file_read( &tmp_file, 0, ( uint8_t* )buffer, tmp_file.size ) ) {
    fat_file_close( &tmp_file );
    return false;
  }
  // close file again
  fat_file_close( &tmp_file );
  // populate return
  dir->entry_list = buffer;
  dir->entry_count = tmp_file.size / sizeof( fat_directory_entry_raw_t );
  dir->fs = fs;
  dir->read_index = 0;
  EARLY_STARTUP_PRINT( "entry_list = %p, entry_count = %"PRIu32"\r\n",
    ( void* )( dir->entry_list ), dir->entry_count )
  // return success
  return true;
}

/**
 * @fn int fat_directory_extract(fat_directory_t*, fat_directory_entry_t*, uint32_t*)
 * @brief Helper to extract directory entry
 *
 * @param dir
 * @param directory_entry
 * @param current_index
 * @return
 */
int fat_directory_extract(
  fat_directory_t* dir,
  fat_directory_entry_t* directory_entry,
  uint32_t* current_index
) {
  // handle invalid start index
  if ( *current_index >= dir->entry_count ) {
    errno = EINVAL;
    return FAT_EXTRACT_ERROR;
  }
  // variables for accessing entries
  fat_directory_entry_raw_t* file_normal = NULL;
  fat_directory_entry_long_raw_t* file_long = NULL;
  for ( uint32_t idx = *current_index; idx < dir->entry_count; idx++ ) {
    file_normal = dir->entry_list + idx;
    // handle end reached
    if ( 0 == file_normal->name[ 0 ] ) {
      return FAT_EXTRACT_END;
    }
    // handle unused
    if ( 0xE5 == file_normal->name[ 0 ] ) {
      *current_index += 1;
      return FAT_EXTRACT_SKIP;
    }
    // handle long filename
    if ( FAT_DIRECTORY_FILE_ATTRIBUTE_LONG_FILE_NAME == file_normal->attributes ) {
      file_long = ( fat_directory_entry_long_raw_t* )file_normal;
      uint8_t order = file_long->order;
      size_t count = 0;
      if ( ( order | 0x40 ) == order ) {
        memset( directory_entry->name, 0, 256 * sizeof( char ) );
        count = ( size_t )( order - 0x40 );
      }
      // end of string
      memset( directory_entry->name, 0, 256 * sizeof( char ) );
      for (size_t j = count; j > 0; j--) {
        file_long = ( fat_directory_entry_long_raw_t* )( dir->entry_list
          + ( idx + j - 1 ) );
        // fill buffer
        directory_entry->name[ ( count - j ) * 13 +  0 ] = file_long->first_five_two_byte[ 0 ];
        directory_entry->name[ ( count - j ) * 13 +  1 ] = file_long->first_five_two_byte[ 2 ];
        directory_entry->name[ ( count - j ) * 13 +  2 ] = file_long->first_five_two_byte[ 4 ];
        directory_entry->name[ ( count - j ) * 13 +  3 ] = file_long->first_five_two_byte[ 6 ];
        directory_entry->name[ ( count - j ) * 13 +  4 ] = file_long->first_five_two_byte[ 8 ];
        directory_entry->name[ ( count - j ) * 13 +  5 ] = file_long->next_six_two_byte[ 0 ];
        directory_entry->name[ ( count - j ) * 13 +  6 ] = file_long->next_six_two_byte[ 2 ];
        directory_entry->name[ ( count - j ) * 13 +  7 ] = file_long->next_six_two_byte[ 4 ];
        directory_entry->name[ ( count - j ) * 13 +  8 ] = file_long->next_six_two_byte[ 6 ];
        directory_entry->name[ ( count - j ) * 13 +  9 ] = file_long->next_six_two_byte[ 8 ];
        directory_entry->name[ ( count - j ) * 13 + 10 ] = file_long->next_six_two_byte[ 10 ];
        directory_entry->name[ ( count - j ) * 13 + 11 ] = file_long->final_two_byte[ 0 ];
        directory_entry->name[ ( count - j ) * 13 + 12 ] = file_long->final_two_byte[ 2 ];
      }
      // Replace with spaces
      for ( size_t j = 0; j < PATH_MAX; j++ ) {
        if ( 0xFF == directory_entry->name[ j ] ) {
          directory_entry->name[ j ] = 0x20;
        }
      }
      // set skip count
      *current_index += count;
      // update idx to skip long file name stuff completely
      idx += count;
      // reset file to entry
      file_normal = dir->entry_list + idx;
    } else {
      // extract small filename
      char display_name[ 13 ];
      memset( display_name, 0, sizeof( char ) * 13 );
      size_t position = 0;
      for ( size_t j = 0; j < 8; j++ ) {
        char c = file_normal->name[ j ];
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
        char c = file_normal->extension[ j ];
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
      // copy to temporary directory_entry->name again
      strncpy( directory_entry->name, display_name, 13 );
    }
    // populate further stuff
    directory_entry->first_cluster = file_normal->first_cluster_lower;
    directory_entry->attributes = file_normal->attributes;
    directory_entry->size = file_normal->file_size;
    // increment index
    *current_index += 1;
    // break out
    break;
  }
  // return success
  return FAT_EXTRACT_SUCCESS;
}

/**
 * @fn bool fat_directory_open_subdirectory(fat_fs_t*, fat_directory_t*, const char*, fat_directory_t*)
 * @brief Helper to open a subdirectory
 *
 * @param fs
 * @param dir
 * @param path
 * @param subdir
 * @return
 */
bool fat_directory_open_subdirectory(
  fat_fs_t* fs,
  fat_directory_t* dir,
  const char* path,
  fat_directory_t* subdir
) {
  fat_directory_entry_t* directory_entry = malloc( sizeof( *directory_entry ) );
  if ( ! directory_entry ) {
    errno = ENOMEM;
    return false;
  }
  // clear out
  memset( directory_entry, 0, sizeof( *directory_entry ) );
  // variables for accessing entries
  fat_extract_t extract;
  uint32_t idx = 0;
  // loop while there is something to loop
  while ( FAT_EXTRACT_END != ( extract = fat_directory_extract(
    dir, directory_entry, &idx
  ) ) ) {
    // handle possible error
    if ( FAT_EXTRACT_ERROR == extract ) {
      free( directory_entry );
      return false;
    }
    // skip unused
    if ( FAT_EXTRACT_SKIP == extract ) {
      continue;
    }
    // check for match
    if (
      strlen( directory_entry->name ) == strlen( path )
      && 0 == strcmp( directory_entry->name, path )
    ) {
      // open directory
      bool open_result = fat_directory_open_sub_directory_by_directory(
        fs, directory_entry, subdir );
      // free directory entry again
      free( directory_entry );
      // return result of open sub dir by dir
      return open_result;
    }
  }
  /*// loop through entries and lookup for a match
  for ( uint32_t idx = 0; idx < dir->entry_count; idx++ ) {
    // extract filename to buffer
    fat_extract_t extract = fat_directory_extract(
      dir, idx, directory_entry, &idx );
    // handle error
    if ( FAT_EXTRACT_ERROR == extract ) {
      free( directory_entry );
      return false;
    // handle skip
    } else if ( FAT_EXTRACT_UNUSED == extract ) {
      continue;
    // handle end reached
    } else if ( FAT_EXTRACT_END == extract ) {
      break;
    }
    // check for match
    if (
      strlen( directory_entry->name ) == strlen( path )
      && 0 == strcmp( directory_entry->name, path )
    ) {
      // open directory
      bool open_result = fat_directory_open_sub_directory_by_directory(
        fs, directory_entry, subdir );
      // free directory entry again
      free( directory_entry );
      // return result of open sub dir by dir
      return open_result;
    }
  }*/
  free( directory_entry );
  // nothing found
  errno = ENOENT;
  return false;
}

/**
 * @fn bool fat_directory_open_sub_directory_by_directory(fat_fs_t*, fat_directory_entry_t*, fat_directory_t*)
 * @brief Helper to open sub directory by directoy entry
 *
 * @param fs
 * @param entry
 * @param subdir
 * @return
 */
bool fat_directory_open_sub_directory_by_directory(
  fat_fs_t* fs,
  fat_directory_entry_t* entry,
  fat_directory_t* subdir
) {
  fat_file_t file;
  // try to open file by entry
  if ( ! fat_file_open_by_directory( fs, entry, &file ) ) {
    return false;
  }
  // handle no block list
  if ( ! file.block_list ) {
    fat_file_close( &file );
    errno = ENOENT;
    return false;
  }
  // allocate buffer
  fat_directory_entry_raw_t* buffer = malloc( file.size );
  if ( ! buffer ) {
    fat_file_close( &file );
    errno = ENOMEM;
    return false;
  }
  // read file to buffer
  if ( !fat_file_read( &file, 0, ( uint8_t* )buffer, file.size ) ) {
    fat_file_close( &file );
    return false;
  }
  // close file again
  fat_file_close( &file );
  // populate subdir
  subdir->entry_list = buffer;
  subdir->entry_count = file.size / sizeof( fat_directory_entry_raw_t );
  subdir->fs = fs;
  subdir->read_index = 0;
  // return success
  return true;
}

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
  fat_fs_t* fs,
  const char* path,
  fat_directory_t* dir
) {
  // check for starting slash, since path has to be absolute
  if ( '/' != path[ 0 ] ) {
    errno = EINVAL;
    return false;
  }
  // duplicate path
  char* pathdup = strdup( path );
  if ( ! pathdup ) {
    errno = ENOMEM;
    return false;
  }
  // try to open root directory
  if ( ! fat_directory_open_root_directory( fs, dir ) ) {
    free( pathdup );
    return false;
  }
  EARLY_STARTUP_PRINT( "pathdup = \"%s\"\r\n", pathdup )
  char* part = strtok( pathdup, PATH_DELIMITER );
  EARLY_STARTUP_PRINT( "part = \"%p\"\r\n", ( void* )part )
  while ( part ) {
    EARLY_STARTUP_PRINT( "part = \"%p\"\r\n", ( void* )part )
    // if part has length, try to open subdir
    if ( 0 != strlen( part ) ) {
      EARLY_STARTUP_PRINT( "part = \"%p\"\r\n", ( void* )part )
      fat_directory_t subdir;
      // try to open subdir
      if ( ! fat_directory_open_subdirectory( fs, dir, part, &subdir ) ) {
        free( pathdup );
        return false;
      }
      // close current directory
      fat_directory_close( dir );
      // overwrite dir with subdir
      memcpy( dir, &subdir, sizeof( fat_directory_t ) );
    }
    EARLY_STARTUP_PRINT( "part = \"%p\"\r\n", ( void* )part )
    // get next
    part = strtok( NULL, PATH_DELIMITER );
  }
  // free duplicate again
  free( pathdup );
  // return success
  return true;
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
void fat_directory_close( fat_directory_t* dir ) {
  if ( dir->entry_list ) {
    free( dir->entry_list );
  }
  dir->entry_list = NULL;
  dir->entry_count = 0;
}
