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
#include <unistd.h>
#include <sys/bolthur.h>
#include "superblock.h"
#include "../dev.h"

/**
 * @fn int extfs_superblock_read(extfs_superblock_t*, mbr_table_entry_t*, const char*)
 * @brief Method to read superblock information
 *
 * @param block
 * @param mbr
 * @param device
 * @return
 */
int extfs_superblock_read(
  extfs_superblock_t* block,
  mbr_table_entry_t* mbr,
  const char* device
) {
  // fetch stat information
  struct stat st;
  if ( -1 == stat( device, &st ) ) {
    EARLY_STARTUP_PRINT( "Unable to fetch device stat for %s\r\n", device )
    return -EIO;
  }
  // get sector size from stat
  off_t sector_size = st.st_blksize;
  if ( 0 == sector_size ) {
    EARLY_STARTUP_PRINT( "Invalid stat information received\r\n" )
    return -EIO;
  }
  // calculate offset ( superblock is located at offset 1024 relative to start )
  off_t read_offset = ( ( off_t )mbr->data.relative_sector * sector_size )
    + 1024;
  // try to read superblock
  int read_result = dev_read(
    block,
    sizeof( extfs_superblock_t ),
    read_offset,
    device
  );
  if ( 0 > read_result ) {
    EARLY_STARTUP_PRINT( "Error while reading superblock!\r\n" )
    return read_result;
  }
  // validate partition
  if ( EXTFS_SUPER_MAGIC != block->s_magic ) {
    EARLY_STARTUP_PRINT(
      "Invalid signature, expected %"PRIx16" but received %"PRIx16"!\r\n",
      EXTFS_SUPER_MAGIC, block->s_magic
    )
    return -EPROTO;
  }
  // return 0 since everything fits
  return 0;
}

/**
 * @fn int extfs_superblock_write(extfs_superblock_t*, mbr_table_entry_t*, const char*)
 * @brief Method to write superblock changes to persistence
 *
 * @param block
 * @param mbr
 * @param device
 * @return
 */
int extfs_superblock_write(
  __unused extfs_superblock_t* block,
  __unused mbr_table_entry_t* mbr,
  __unused const char* device
) {
  return -ENOSYS;
}
