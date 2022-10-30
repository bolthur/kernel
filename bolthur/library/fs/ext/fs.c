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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../ext.h"
#include <inttypes.h>
#include <sys/bolthur.h>

/**
 * @fn ext_fs_t ext_fs_init*(dev_read_t, dev_write_t, uint32_t)
 * @brief
 *
 * @param read
 * @param write
 * @param offset
 * @return
 *
 * @exception ENOMEM some allocation internally failed
 */
ext_fs_t* ext_fs_init(
  dev_read_t read,
  dev_write_t write,
  uint32_t offset
) {
  EARLY_STARTUP_PRINT( "offset = %"PRIu32"\r\n", offset );
  // allocate data block
  ext_fs_t* data = malloc( sizeof( *data ) );
  if ( ! data ) {
    errno = ENOMEM;
    return NULL;
  }
  // clear space
  memset( data, 0, sizeof( *data ) );
  // populate callbacks
  data->dev_read = read;
  data->dev_write = write;
  data->cache_construct = cache_construct;
  data->cache_destruct = cache_destruct;
  data->cache_sync = ext_cache_sync;
  data->cache_block_allocate = ext_cache_block_allocate;
  data->cache_block_release = ext_cache_block_release;
  data->cache_block_dirty = ext_cache_block_dirty;
  // populate remaining properties
  data->partition_sector_offset = offset;
  // return data
  return data;
}

/**
 * @fn bool ext_fs_mount(ext_fs_t*)
 * @brief Wrapper to mount a ext filesystem
 *
 * @param fs
 * @return
 *
 * @exception ENOMEM Some allocation failed
 * @exception EIO Reading necessary data from disk failed
 * @exception EINVAL Invalid fs parameter passed
 */
bool ext_fs_mount( ext_fs_t* fs ) {
  // handle invalid
  if ( ! fs || fs->superblock || fs->boot_sector ) {
    errno = EINVAL;
    return false;
  }
  // allocate superblock
  fs->superblock = malloc( sizeof( ext_superblock_t ) );
  if ( ! fs->superblock ) {
    errno = ENOMEM;
    return false;
  }
  // clear space
  memset( fs->superblock, 0, sizeof( ext_superblock_t ) );
  // read root superblock
  if ( ! ext_superblock_read( fs, fs->superblock ) ) {
    free( fs->superblock );
    fs->superblock = NULL;
    return false;
  }
  // allocate boot sector data
  fs->boot_sector = malloc( sizeof( char ) * 1024 );
  if ( ! fs->boot_sector ) {
    errno = ENOMEM;
    free( fs->superblock );
    fs->superblock = NULL;
    return false;
  }
  // clear space
  memset( fs->boot_sector, 0, sizeof( char ) * 1024 );
  // read boot sector data
  if ( ! fs->dev_read(
    ( uint32_t* )fs->boot_sector,
    sizeof( char ) * 1024,
    fs->partition_sector_offset
  ) ) {
    errno = EIO;
    free( fs->superblock );
    free( fs->boot_sector );
    fs->boot_sector = NULL;
    fs->superblock = NULL;
    return false;
  }
  // create cache
  fs->handle = fs->cache_construct(
    fs, ext_superblock_block_size( fs->superblock ) );
  if ( ! fs->handle ) {
    errno = ENOMEM;
    free( fs->superblock );
    free( fs->boot_sector );
    fs->boot_sector = NULL;
    fs->superblock = NULL;
    return false;
  }
  // debug dump
  ext_superblock_dump( fs->superblock );
  //// FIXME: FILL STUFF
  // return success
  return true;
}

/**
 * @fn bool ext_fs_unmount(ext_fs_t*)
 * @brief Unmount file system properly
 *
 * @param fs
 * @return
 *
 * @exception EIO write back of super block failed
 */
bool ext_fs_unmount( ext_fs_t* fs) {
  if ( ! ext_superblock_write( fs, fs->superblock ) ) {
    errno = EIO;
    return false;
  }
  // free cached data
  free( fs->boot_sector );
  free( fs->superblock );
  // sync cache
  fs->cache_sync( fs->handle );
  // cleanup cache
  fs->cache_destruct( fs->handle );
  // set boot sector, superblock and handle to null again
  fs->boot_sector = NULL;
  fs->superblock = NULL;
  fs->handle = NULL;
  // return success
  return true;
}

/**
 * @fn bool ext_fs_sync(ext_fs_t*)
 * @brief Wrapper to sync
 *
 * @param fs
 * @return
 */
bool ext_fs_sync( ext_fs_t* fs ) {
  return fs->cache_sync( fs->handle );
}
