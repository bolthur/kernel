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
#include "../fat.h"
#include <inttypes.h>
#include <sys/bolthur.h>

/**
 * @fn bool fat_cluster_next(fat_fs_t*, uint32_t, uint32_t*)
 * @brief Helper to get next cluster by current
 *
 * @param fs
 * @param current
 * @param next
 * @return
 */
bool fat_cluster_next( fat_fs_t* fs, uint32_t current, uint32_t* next ) {
  uint32_t sector_size = fs->boot_sector->bytes_per_sector;
  uint32_t fat_sector = 0;
  uint32_t fat_offset = 0;
  // determine fat offset
  if ( FAT_FAT12 == fs->type ) {
    fat_offset = current * 12;
    fat_sector = fat_offset / ( sector_size * 8 );
    fat_offset %= ( sector_size * 8 );
  } else if ( FAT_FAT16 == fs->type ) {
    fat_offset = ( current * sizeof( uint16_t ) ) % sector_size;
    fat_sector = ( current * sizeof( uint16_t ) ) / sector_size;
  } else if ( FAT_FAT32 == fs->type || FAT_EXFAT == fs->type ) {
    fat_offset = ( current * sizeof( uint32_t ) ) % sector_size;
    fat_sector = ( current * sizeof( uint32_t ) ) / sector_size;
  } else {
    errno = EINVAL;
    return false;
  }
  // calculate sector to load
  fat_sector += fs->fat_data.first_fat_sector;
  // get fat cache
  cache_block_t* fat_cache = fs->cache_block_allocate( fs->handle, fat_sector, true );
  if ( ! fat_cache ) {
    errno = ENOMEM;
    return false;
  }
  // get next cluster
  uint32_t next_cluster;
  if ( FAT_FAT12 == fs->type ) {
    next_cluster = ( uint32_t )( *( ( uint16_t* )&fat_cache->data[ fat_offset / 8 ] ) );
  } else if ( FAT_FAT16 == fs->type ) {
    next_cluster = ( uint32_t )( *( ( uint16_t* )&fat_cache->data[ fat_offset ] ) );
  } else if ( FAT_FAT32 == fs->type || FAT_EXFAT == fs->type ) {
    next_cluster = *( ( uint32_t* )&fat_cache->data[ fat_offset ] );
  } else {
    errno = EINVAL;
    return false;
  }
  // ignore top most 4 bit for fat32
  if ( FAT_FAT32 == fs->type ) {
    next_cluster &= 0x0FFFFFFF;
  }
  // write next cluster
  *next = next_cluster;
  // free cache block again
  fs->cache_block_release( fat_cache, false );
  // return success
  return true;
}

/**
 * @fn uint32_t fat_cluster_get_by_file(fat_file_t*, uint32_t)
 * @brief Helper to get next cluster of file
 *
 * @param file
 * @param cluster_index
 * @return
 */
uint32_t fat_cluster_get_by_file( fat_file_t* file, uint32_t cluster_index ) {
  // no more indizes? => return zero
  if ( cluster_index > file->block_count ) {
    return 0;
  }
  // return cluster by index
  return file->block_list[ cluster_index ];
}

/**
 * @fn uint32_t fat_cluster_get_offset(fat_fs_t*, uint32_t)
 * @brief Get absolute cluster offset
 *
 * @param fs
 * @param cluster_index
 * @return
 */
uint32_t fat_cluster_get_offset( fat_fs_t* fs, uint32_t cluster_index ) {
  // calculate offset
  uint32_t cluster_offset = (
    ( fs->boot_sector->sectors_per_cluster * ( cluster_index - 2 )
  ) + fs->fat_data.first_data_sector );
  // return offset
  return cluster_offset;
}

/**
 * @fn bool fat_cluster_read(fat_fs_t*, uint8_t*, uint32_t, uint32_t, uint32_t)
 * @brief Helper to read a cluster
 *
 * @param fs
 * @param buffer
 * @param cluster_index
 * @param offset
 * @param length
 * @return
 */
bool fat_cluster_read(
  fat_fs_t* fs,
  uint8_t* buffer,
  uint32_t cluster_index,
  uint32_t offset,
  uint32_t length
) {
  // get cluster size
  uint32_t cluster_size = fs->boot_sector->sectors_per_cluster *
    fs->boot_sector->bytes_per_sector;
  // check limits
  if ( cluster_size < offset + length ) {
    errno = EINVAL;
    return false;
  }
  // calculate cluster offset
  uint32_t cluster_offset = fat_cluster_get_offset( fs, cluster_index );
  EARLY_STARTUP_PRINT( "Reading from %#"PRIx32"\r\n", cluster_offset )
  // load sector to cache
  cache_block_t* cache = fs->cache_block_allocate( fs->handle, cluster_offset, true );
  if ( ! cache ) {
    errno = ENOMEM;
    return false;
  }
  // copy over
  memcpy( buffer, cache->data + offset, length );
  // free cache block again
  fs->cache_block_release( cache, false );
  // return success
  return true;
}
