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
#include "../ext.h"
#include <inttypes.h>
#include <sys/bolthur.h>

/**
 * @fn bool ext_superblock_read(ext_generic_data_t*, ext_superblock_t*)
 * @brief
 *
 * @param fs filesystem relevant data
 * @param superblock address where to save superblock
 * @return
 *
 * @exception EINVAL invalid data parameter
 * @exception EIO Reading superblock failed
 * @exception EPROTO invalid magic in super block
 */
bool ext_superblock_read( ext_fs_t* fs, ext_superblock_t* superblock ) {
  // validate data
  if ( ! fs || ! fs->dev_read ) {
    errno = EINVAL;
    return false;
  }
  // try to read superblock
  if ( ! fs->dev_read(
    ( uint32_t* )superblock,
    sizeof( ext_superblock_t ),
    // superblock is located at offset 1024
    fs->partition_sector_offset + 1024
  ) ) {
    EARLY_STARTUP_PRINT( "UNABLE TO READ SUPERBLOCK!\r\n" )
    errno = EIO;
    return false;
  }
  // validate partition
  if ( EXT_SUPER_MAGIC != superblock->s_magic ) {
    EARLY_STARTUP_PRINT(
      "Invalid signature, expected %"PRIx16" but received %"PRIx16"!\r\n",
      EXT_SUPER_MAGIC, superblock->s_magic
    )
    errno = EPROTO;
    return false;
  }

  uint32_t total_group_by_blocks = ext_superblock_total_group_by_blocks(
    superblock
  );
  // calculate total inode per group
  uint32_t total_group_by_inode = ext_superblock_total_group_by_inode(
    superblock
  );
  // values must match
  if ( total_group_by_blocks != total_group_by_inode ) {
    EARLY_STARTUP_PRINT(
      "total group count and total inode count don't match ( %"PRIu32" != %"PRIu32" )\r\n",
      total_group_by_blocks, total_group_by_inode )
      errno = EFAULT;
      return false;
  }
  EARLY_STARTUP_PRINT( "Signature: %"PRIx16"!\r\n", superblock->s_magic )
  // return success
  return true;
}

/**
 * @fn bool ext_superblock_write(ext_fs_t*, ext_superblock_t*)
 * @brief Helper to write superblock to disk
 *
 * @param data
 * @param superblock
 * @return
 */
bool ext_superblock_write( ext_fs_t* fs, ext_superblock_t* superblock ) {
  // get count of groups where an update might be necessary
  uint32_t group_count = ( superblock->s_blocks_count + superblock->s_blocks_per_group - 1 ) /
    superblock->s_blocks_per_group;
  uint32_t block_size = ext_superblock_block_size( superblock );
  size_t copy = sizeof( ext_superblock_t );
  // loop and update block if necessary
  for ( uint32_t idx = 0; idx < group_count; idx++ ) {
    // skip if there is no superblock backup
    if ( ! ext_blockgroup_has_superblock( superblock, idx ) ) {
      continue;
    }
    // default start is 1024 => idx = 0 => root superblock
    uint32_t start = 1024;
    // handle superblock backup
    if ( 0 < idx ) {
      // get block group offset
      uint32_t block_group_offset = idx * fs->superblock->s_blocks_per_group;
      // calculate start
      start = ( fs->superblock->s_first_data_block + block_group_offset )
        * block_size;
    }
    // create cache without reading data
    cache_block_t* cache = fs->cache_block_allocate(
      fs->handle, start / block_size, false
    );
    // handle error
    if ( ! cache ) {
      return false;
    }
    // handle root superblock
    if ( 0 == idx ) {
      // memory copy offset
      uint32_t offset = 0;
      // handle bigger block sizes ( bootsector is stored there )
      if ( 1024 < block_size ) {
        // copy boot sector
        memcpy( cache->data, fs->boot_sector, 1024 );
        // set offset
        offset = 1024;
        // clear out rest at the end
        memset( cache->data + offset + copy, 0, block_size - offset - copy );
      }
      // copy superblock data itself
      memcpy( cache->data + offset, superblock, copy );
    // handle superblock backup ( copy superblock and erase rest of block )
    } else {
      // copy superblock
      memcpy( cache->data, superblock, copy );
      // erase rest of block
      memset( cache->data + copy, 0, block_size - copy );
    }
    // release cache again with dirty flag to force write back
    fs->cache_block_release( cache, true );
  }
  return false;
}

/**
 * @fn size_t ext_ext_superblock_block_size(ext_superblock_t*)
 * @brief Helper to extract block size from superblock
 *
 * @param superblock
 * @return
 */
uint32_t ext_superblock_block_size( ext_superblock_t* superblock ) {
  return 1024 << superblock->s_log_block_size;
}

/**
 * @fn size_t ext_ext_superblock_frag_size(ext_superblock_t*)
 * @brief Helper to extract fragment size from superblock
 *
 * @param superblock
 * @return
 */
uint32_t ext_superblock_frag_size( ext_superblock_t* superblock ) {
  return 1024 << superblock->s_log_frag_size;
}

/**
 * @fn uint32_t ext_ext_superblock_total_group_by_blocks(ext_superblock_t*)
 * @brief Helper to get total block groups by block information
 *
 * @param superblock
 * @return
 */
uint32_t ext_superblock_total_group_by_blocks( ext_superblock_t* superblock ) {
  return superblock->s_blocks_count / superblock->s_blocks_per_group +
    ( superblock->s_blocks_count % superblock->s_blocks_per_group ? 1 : 0 );
}

/**
 * @fn uint32_t ext_superblock_total_inode(ext_superblock_t*)
 * @brief Helper to get total group by inode information
 *
 * @param superblock
 * @return
 */
uint32_t ext_superblock_total_group_by_inode( ext_superblock_t* superblock ) {
  return superblock->s_inodes_count / superblock->s_inodes_per_group +
    ( superblock->s_inodes_count % superblock->s_inodes_per_group ? 1 : 0 );
}

/**
 * @fn uint32_t ext_superblock_start(ext_superblock_t*)
 * @brief Method to get superblock start offset
 *
 * @param superblock
 * @return
 */
uint32_t ext_superblock_start( ext_superblock_t* superblock ) {
  return 1024 < ext_superblock_block_size( superblock ) ? 0 : 1;
}

/**
 * @fn uint32_t ext_superblock_inode_size(ext_superblock_t*)
 * @brief Helper to get inode size
 *
 * @param superblock
 * @return
 */
uint32_t ext_superblock_inode_size( ext_superblock_t* superblock ) {
  return 0 == superblock->s_rev_level ? 128 : superblock->s_inode_size;
}

/**
 * @fn void ext_superblock_dump(ext_superblock_t*)
 * @brief Dump super block data
 *
 * @param superblock
 */
void ext_superblock_dump( ext_superblock_t* superblock ) {
  EARLY_STARTUP_PRINT( "superblock->s_inodes_count = %"PRIu32"\r\n", superblock->s_inodes_count )
  EARLY_STARTUP_PRINT( "superblock->s_blocks_count = %"PRIu32"\r\n", superblock->s_blocks_count )
  EARLY_STARTUP_PRINT( "superblock->s_r_blocks_count = %"PRIu32"\r\n", superblock->s_r_blocks_count )
  EARLY_STARTUP_PRINT( "superblock->s_free_blocks_count = %"PRIu32"\r\n", superblock->s_free_blocks_count )
  EARLY_STARTUP_PRINT( "superblock->s_free_inodes_count = %"PRIu32"\r\n", superblock->s_free_inodes_count )
  EARLY_STARTUP_PRINT( "superblock->s_first_data_block = %"PRIu32"\r\n", superblock->s_first_data_block )
  EARLY_STARTUP_PRINT( "superblock->s_log_block_size = %"PRIu32"\r\n", superblock->s_log_block_size )
  EARLY_STARTUP_PRINT( "superblock->s_blocks_per_group = %"PRIu32"\r\n", superblock->s_blocks_per_group )
  EARLY_STARTUP_PRINT( "superblock->s_inodes_per_group = %"PRIu32"\r\n", superblock->s_inodes_per_group )
  EARLY_STARTUP_PRINT( "superblock->s_magic = %"PRIu16"\r\n", superblock->s_magic )
  EARLY_STARTUP_PRINT( "superblock->s_rev_level = %"PRIu32"\r\n", superblock->s_rev_level )
  EARLY_STARTUP_PRINT( "superblock->s_inode_size = %"PRIu16"\r\n", superblock->s_inode_size )
  EARLY_STARTUP_PRINT( "superblock->s_feature_compat = %"PRIu32"\r\n", superblock->s_feature_compat )
  EARLY_STARTUP_PRINT( "superblock->s_feature_incompat = %"PRIu32"\r\n", superblock->s_feature_incompat )
  EARLY_STARTUP_PRINT( "superblock->s_feature_ro_incompat = %"PRIu32"\r\n", superblock->s_feature_ro_incompat )
  EARLY_STARTUP_PRINT( "ext_superblock_block_size( %p ) = %"PRIu32"\r\n",
    ( void* )superblock, ext_superblock_block_size( superblock ) )
}
