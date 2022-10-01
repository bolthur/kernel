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
