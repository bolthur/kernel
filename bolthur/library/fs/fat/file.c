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
 * @fn bool fat_file_open(fat_fs_t*, const char*, fat_file_t*)
 * @brief Open a file
 *
 * @param fs
 * @param path
 * @param file
 * @return
 */
bool fat_file_open(
  __unused fat_fs_t* fs,
  __unused const char* path,
  __unused fat_file_t* file
) {
  errno = ENOSYS;
  return false;
}

/**
 * @fn bool fat_file_open_by_directory(fat_fs_t*, fat_directory_entry_t*, fat_file_t*)
 * @brief Open a file by directory
 *
 * @param fs
 * @param dir
 * @param file
 * @return
 */
bool fat_file_open_by_directory(
  fat_fs_t* fs,
  fat_directory_entry_t* dir,
  fat_file_t* file
) {
  // get cluster size
  uint32_t cluster_size = fs->boot_sector->bytes_per_sector
    * fs->boot_sector->sectors_per_cluster;
  // prepare file structure
  file->fs = fs;
  file->size = dir->size;
  memcpy( &file->entry, dir, sizeof( fat_directory_entry_t ) );
  // erase block list stuff for empty files
  if ( 0 == dir->first_cluster ) {
    file->block_list = NULL;
    file->block_count = 0;
  }
  // in case file size is greater than 0, calculate cluster count, else ( directories ) assume 1
  file->block_count = 1;
  if ( 0 < file->size ) {
    file->block_count = FAT_CLUSTER_COUNT( file->size, cluster_size );
  }
  // allocate block list
  file->block_list = malloc( sizeof( uint32_t ) * file->block_count );
  if ( ! file->block_list ) {
    errno = ENOMEM;
    return false;
  }
  // clear out
  memset( file->block_list, 0, sizeof( uint32_t ) * file->block_count );
  // populate first cluster
  file->block_list[ 0 ] = dir->first_cluster;
  // load cluster blocks
  uint32_t current_cluster;
  uint32_t next_cluster;
  uint32_t idx = 1;
  do {
    // set current cluster and reset next cluster
    current_cluster = file->block_list[ idx - 1 ];
    next_cluster = 0;
    // try to get next cluster
    if ( ! fat_cluster_next( fs, current_cluster, &next_cluster ) ) {
      return false;
    }
    EARLY_STARTUP_PRINT(
      "current_cluster = %#"PRIx32", next_cluster = %#"PRIx32"\r\n",
      current_cluster, next_cluster )
    // skip if there is no next cluster
    if ( 0x0FFFFFF8 <= next_cluster ) {
      continue;
    }
    // reallocate block list
    if ( idx >= file->block_count ) {
      // increase size
      file->block_count += 1;
      // realloc
      uint32_t* block_list = realloc(
        file->block_list,
        sizeof( uint32_t ) * file->block_count
      );
      // handle error
      if ( ! block_list ) {
        // clear block list on error
        if ( file->block_list ) {
          free( file->block_list );
          file->block_list = NULL;
          file->block_count = 0;
        }
        errno = ENOMEM;
        return false;
      }
      // overwrite
      file->block_list = block_list;
    }
    // fill cluster into block list
    file->block_list[ idx++ ] = next_cluster;
  } while ( 0x0FFFFFF8 > next_cluster );
  // adjust file size if 0
  if ( 0 == file->size ) {
    file->size = file->block_count * cluster_size;
  }
  // return success
  return true;
}
/**
 * @fn bool fat_file_read(fat_file_t*, size_t, uint8_t*, size_t)
 * @brief Read opened file with offset
 *
 * @param file
 * @param offset
 * @param buffer
 * @param length
 * @return
 */
bool fat_file_read(
  fat_file_t* file,
  size_t offset,
  uint8_t* buffer,
  size_t length
) {
  // handle exceed of offset compared to real size
  if ( offset > file->size ) {
    errno = EIO;
    return false;
  }
  // get cluster size
  uint32_t cluster_size = file->fs->boot_sector->sectors_per_cluster
    * file->fs->boot_sector->bytes_per_sector;
  size_t remaining = length;
  uint8_t* buffer_copy = buffer;
  uint32_t cluster_index = offset / cluster_size;

  // handle cluster offset at the beginning
  uint32_t offset_in_cluster = offset & ( cluster_size - 1 );
  if ( offset_in_cluster ) {
    // get cluster
    uint32_t cluster = fat_cluster_get_by_file( file, cluster_index++ );
    // handle error
    if ( 0 == cluster ) {
      errno = EIO;
      return false;
    }
    // variable for loaded count
    uint32_t count = FAT_MIN(remaining, cluster_size - offset_in_cluster);
    if ( ! fat_cluster_read(
      file->fs,
      buffer_copy,
      cluster,
      offset_in_cluster,
      count
    ) ) {
      return false;
    }
    // increment buffer and decrement remaining
    buffer_copy += count;
    remaining -= count;
  }

  // load in the middle
  while ( remaining >= cluster_size ) {
    // get cluster
    uint32_t cluster = fat_cluster_get_by_file( file, cluster_index++ );
    // handle error
    if ( 0 == cluster ) {
      errno = EIO;
      return false;
    }
    // variable for loaded count
    if ( ! fat_cluster_read(
      file->fs,
      buffer_copy,
      cluster,
      0,
      cluster_size
    ) ) {
      return false;
    }
    // increment buffer and decrement remaining
    buffer_copy += cluster_size;
    remaining -= cluster_size;
  }

  // handle offset at the end
  if ( remaining ) {
    // get cluster
    uint32_t cluster = fat_cluster_get_by_file( file, cluster_index );
    // handle error
    if ( 0 == cluster ) {
      errno = EIO;
      return false;
    }
    // variable for loaded count
    if ( ! fat_cluster_read(
      file->fs,
      buffer_copy,
      cluster,
      0,
      remaining
    ) ) {
      return false;
    }
    // increment buffer and decrement remaining
    buffer_copy += remaining;
    remaining -= remaining;
  }
  // return success
  return true;
}

/**
 * @fn void fat_file_close(fat_file_t*)
 * @brief Close opened file
 *
 * @param file
 */
void fat_file_close( fat_file_t* file ) {
  if ( file->block_list ) {
    free( file->block_list );
  }
  file->block_list = NULL;
  file->block_count = 0;
}
