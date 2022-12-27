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
#include "extfs/superblock.h"
#include "probe.h"

/**
 * @fn int probe(const char*, mbr_table_entry_t*)
 * @brief Probe filesystem
 *
 * @param device
 * @param entry
 * @return
 */
int probe( const char* device, mbr_table_entry_t* entry ) {
  // allocate space for super block
  extfs_superblock_t* block = malloc( sizeof( *block ) );
  // handle error
  if ( ! block ) {
    return -ENOMEM;
  }
  // clear out memory
  memset( block, 0, sizeof( *block ) );
  // read superblock
  int result = extfs_superblock_read( block, entry, device );
  // handle error
  if ( 0 != result ) {
    free( block );
    return result;
  }
  // free memory
  free( block );
  // return 0 as success
  return 0;
}
