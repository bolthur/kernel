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

#include <math.h>
#include <string.h>
#include "../ext.h"

/**
 * @fn bool is_power_of(uint32_t, uint32_t)
 * @brief Helper to check whether num is power of exponent
 *
 * @param num
 * @param exp
 * @return
 */
static bool is_power_of( uint32_t num, uint32_t exp ) {
  // compute power
  double p = log10( ( double )num ) / log10( ( double )exp );
  // return result
  return 0 == p - ( int32_t )p;
}

/**
 * @fn bool ext_blockgroup_has_superblock(ext_superblock_t*, uint32_t)
 * @brief Helper to check if given block group has a superblock backup
 *
 * @param superblock
 * @param blockgroup
 * @return
 */
bool ext_blockgroup_has_superblock(
  ext_superblock_t* superblock,
  uint32_t group
) {
  // check for sparse super is not set => all block groups contain superblock backup
  if ( ! ( superblock->s_feature_compat & EXT_FEATURE_RO_COMPAT_SPARSE_SUPER ) ) {
    return true;
  }
  // if sparse super is active, backup of superblock is only within a few groups
  return
    // groups 0 and 1 contain a backup
    1 >= group
    // other possibilities: block group is multiple of 3, 5 or 7
    || is_power_of( group, 3 )
    || is_power_of( group, 5 )
    || is_power_of( group, 7 );
}

/**
 * @fn bool ext_blockgroup_read(ext_fs_t*, uint32_t, ext_blockgroup_t*)
 * @brief Helper to read a block group entry
 *
 * @param fs
 * @param group_number
 * @param blockgroup
 * @return
 */
bool ext_blockgroup_read(
  ext_fs_t* fs,
  uint32_t group_number,
  ext_blockgroup_t* blockgroup
) {
  // get block size
  uint32_t block_size = ext_superblock_block_size( fs->superblock );
  // calculate block number
  uint32_t block_number = ext_superblock_start( fs->superblock )
    + 1 + ( group_number * sizeof( ext_blockgroup_t ) ) / block_size;
  // load cache via handle
  cache_block_t* cache = fs->cache_block_allocate( fs->handle, block_number, true );
  // handle error
  if ( ! cache ) {
    return NULL;
  }
  // copy content and return success
  memcpy(
    blockgroup,
    cache->data + ( ( group_number * sizeof( ext_blockgroup_t ) ) % block_size ),
    sizeof( ext_blockgroup_t )
  );
  // free cache again
  fs->cache_block_release( cache, false );
  // return success
  return true;
}

/**
 * @fn bool ext_blockgroup_write(ext_fs_t*, uint32_t, ext_blockgroup_t*)
 * @brief Helper to write blockgroup to disk
 *
 * @param fs
 * @param group_number
 * @param blockgroup
 * @return
 *
 * @todo implement function
 */
bool ext_blockgroup_write(
  __unused ext_fs_t* fs,
  __unused uint32_t group_number,
  __unused ext_blockgroup_t* blockgroup
) {
  return false;
}
