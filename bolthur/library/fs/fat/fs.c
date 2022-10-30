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
#include "../fat.h"
#include <inttypes.h>
#include <sys/bolthur.h>

/**
 * @fn fat_fs_t fat_fs_init*(dev_read_t, dev_write_t, uint32_t)
 * @brief Generic fat fs init
 *
 * @param read
 * @param write
 * @param offset
 * @return
 */
fat_fs_t* fat_fs_init(
  dev_read_t read,
  dev_write_t write,
  uint32_t offset
) {
  EARLY_STARTUP_PRINT( "offset = %"PRIu32"\r\n", offset );
  // allocate data block
  fat_fs_t* data = malloc( sizeof( *data ) );
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
  data->cache_sync = fat_cache_sync;
  data->cache_block_allocate = fat_cache_block_allocate;
  data->cache_block_release = fat_cache_block_release;
  data->cache_block_dirty = fat_cache_block_dirty;
  // populate remaining properties
  data->partition_sector_offset = offset;
  // return data
  return data;
}

/**
 * @fn bool fat_fs_mount(fat_fs_t*)
 * @brief Wrapper to mount a ext filesystem
 *
 * @param fs
 * @return
 *
 * @exception ENOMEM Some allocation failed
 * @exception EIO Reading necessary data from disk failed
 * @exception EINVAL Invalid fs parameter passed
 */
bool fat_fs_mount( fat_fs_t* fs ) {
  // handle invalid
  if ( ! fs || fs->boot_sector ) {
    errno = EINVAL;
    return false;
  }
  // allocate boot sector data
  fs->boot_sector = malloc( sizeof( fat_bpb_t ) );
  if ( ! fs->boot_sector ) {
    errno = ENOMEM;
    return false;
  }
  // clear space
  memset( fs->boot_sector, 0, sizeof( fat_bpb_t ) );
  // read boot sector data
  if ( ! fs->dev_read(
    ( uint32_t* )fs->boot_sector,
    sizeof( fat_bpb_t ),
    fs->partition_sector_offset
  ) ) {
    errno = EIO;
    free( fs->boot_sector );
    fs->boot_sector = NULL;
    return false;
  }
  // check for valid magic number
  if ( 0xAA55 != fs->boot_sector->boot_sector_signature ) {
    errno = EINVAL;
    free( fs->boot_sector );
    fs->boot_sector = NULL;
    return false;
  }
  // create cache
  fs->handle = fs->cache_construct( fs, fs->boot_sector->bytes_per_sector );
  if ( ! fs->handle ) {
    errno = ENOMEM;
    free( fs->boot_sector );
    fs->boot_sector = NULL;
    return false;
  }
  // populate bunch of data information
  fs->fat_data.total_sectors = 0 == fs->boot_sector->total_sectors_16
    ? fs->boot_sector->total_sectors_32 : fs->boot_sector->total_sectors_16;
  fs->fat_data.fat_size = 0 == fs->boot_sector->table_size_16
    ? fs->boot_sector->extended.fat32.table_size_32 : fs->boot_sector->table_size_16;
  fs->fat_data.root_dir_sectors = ( uint32_t )( ( ( fs->boot_sector->root_entry_count * 32 )
    + ( fs->boot_sector->bytes_per_sector - 1) ) / fs->boot_sector->bytes_per_sector );
  fs->fat_data.first_data_sector = fs->boot_sector->reserved_sector_count + (
    fs->boot_sector->table_count * fs->fat_data.fat_size ) + fs->fat_data.root_dir_sectors;
  fs->fat_data.first_fat_sector = fs->boot_sector->reserved_sector_count;
  fs->fat_data.data_sectors = fs->fat_data.total_sectors - fs->fat_data.first_data_sector;
  fs->fat_data.total_clusters = fs->fat_data.data_sectors / fs->boot_sector->sectors_per_cluster;
  // determine fat type
  if ( 0 == fs->boot_sector->bytes_per_sector ) {
    fs->type = FAT_EXFAT;
    EARLY_STARTUP_PRINT( "detected exfat\r\n" )
  } else if ( 4085 > fs->fat_data.total_clusters ) {
    fs->type = FAT_FAT12;
    EARLY_STARTUP_PRINT( "detected fat12\r\n" )
  } else if ( 65525 > fs->fat_data.total_clusters ) {
    fs->type = FAT_FAT16;
    EARLY_STARTUP_PRINT( "detected fat16\r\n" )
  } else {
    fs->type = FAT_FAT32;
    EARLY_STARTUP_PRINT( "detected fat32\r\n" )
  }
  // everything else except fat32 is not supported yet
  if ( fs->type != FAT_FAT32 ) {
    errno = ENOSYS;
    free( fs->boot_sector );
    fs->boot_sector = NULL;
    return false;
  }
  //// FIXME: FILL STUFF
  // return success
  return true;
}

/**
 * @fn bool fat_fs_unmount(fat_fs_t*)
 * @brief Unmount file system properly
 *
 * @param fs
 * @return
 *
 * @exception EIO write back of super block failed
 */
bool fat_fs_unmount( fat_fs_t* fs) {
  // free cached data
  free( fs->boot_sector );
  // sync cache
  fs->cache_sync( fs->handle );
  // cleanup cache
  fs->cache_destruct( fs->handle );
  // set boot sector and handle to null again
  fs->boot_sector = NULL;
  fs->handle = NULL;
  // return success
  return true;
}

/**
 * @fn bool fat_fs_sync(fat_fs_t*)
 * @brief Wrapper to sync
 *
 * @param fs
 * @return
 */
bool fat_fs_sync( fat_fs_t* fs ) {
  return fs->cache_sync( fs->handle );
  return false;
}
