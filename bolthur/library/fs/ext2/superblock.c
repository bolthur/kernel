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
#include <inttypes.h>
#include <sys/bolthur.h>
#include "../ext2.h"

int32_t ext2_superblock_read(
  device_read_t read,
  ext2_superblock_t* superblock,
  uint32_t offset
) {
  // try to read superblock
  if ( ! read( ( uint32_t* )superblock, sizeof( ext2_superblock_t ), offset ) ) {
    EARLY_STARTUP_PRINT( "UNABLE TO READ SUPERBLOCK!\r\n" )
    return -EIO;
  }
  // validate partition
  if ( EXT2_SUPER_MAGIC != superblock->s_magic ) {
    EARLY_STARTUP_PRINT(
      "Invalid signature, expected %"PRIx16" but received %"PRIx16"!\r\n",
      EXT2_SUPER_MAGIC, superblock->s_magic
    )
    return -EINVAL;
  }
  EARLY_STARTUP_PRINT( "Signature: %"PRIx16"!\r\n", superblock->s_magic )
  // return success
  return 0;
}
