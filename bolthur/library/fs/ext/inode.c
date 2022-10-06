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
#include <stdlib.h>
#include "../ext.h"
#include <inttypes.h>
#include <sys/bolthur.h>

typedef enum {
  EXT_BLOCK_OFFSET_READ_ONLY = 0,
  EXT_BLOCK_OFFSET_ALLOCATE = 1,
  EXT_BLOCK_OFFSET_RESIZE = 2
} block_offset_t;

/**
 * @fn uint32_t inode_block_indirection_level(ext_inode_t*, uint32_t, uint32_t*, uint32_t*)
 * @brief Helper to get inode block indirection level data
 *
 * @param inode
 * @param block
 * @param direct_block
 * @param indirect_block
 * @return
 */
static uint32_t inode_block_indirection_level(
  ext_inode_t* inode,
  uint32_t block,
  uint32_t* direct_block,
  uint32_t* indirect_block
) {
  uint32_t block_size = ext_superblock_block_size( inode->fs->superblock );
  // no indirection
  if ( block < 12 ) {
    *direct_block = block;
    *indirect_block = 0;
    return 0;
  // singly indirection
  } else if ( ( block_size / 4 + 12 ) > block ) {
    *direct_block = 12;
    *indirect_block = block - 12;
    return 1;
  // doubly indirection
  } else if ( ( block_size / 4 ) * ( block_size / 4 + 1 ) + 12 > block ) {
    *direct_block = 13;
    *indirect_block = block - 12 - ( block_size / 4 );
    return 2;
    // triply indirection
  } else {
    *direct_block = 14;
    *indirect_block = block - 12 - ( block_size / 4 ) * ( block_size / 4 + 1);
    return 3;
  }
}

/**
 * @fn uint32_t inode_block_get_offset(ext_inode_t*, uint32_t, block_offset_t)
 * @brief Helper to get inode block offset
 *
 * @param inode
 * @param block
 * @param offset_handling
 * @return
 *
 * @todo add offset_handling to complete function
 */
static uint32_t inode_block_get_offset(
  ext_inode_t* inode,
  uint32_t block,
  __unused block_offset_t offset_handling
) {
  uint32_t block_size = ext_superblock_block_size( inode->fs->superblock );
  uint32_t direct_block = 0;
  uint32_t indirect_block = 0;
  // get indirection level
  uint32_t level = inode_block_indirection_level(
    inode, block, &direct_block, &indirect_block );
  // read direct block number or number of first table block from inode
  uint32_t block_number = inode->inode->i_block[ direct_block ];
  // evaluate indirect blocks
  for ( uint32_t idx = 0; idx < level && block_number; idx++ ) {
    // allocate new cache block with content read
    cache_block_t* cache = inode->fs->cache_block_allocate(
      inode->fs->handle, block_number, true );
    // handle error
    if ( ! cache ) {
      return 0;
    }
    // get table
    uint32_t* table = ( uint32_t* )cache->data;
    // determine pow
    uint32_t pow_result = ( uint32_t )lround( pow(
      ( double )( block_size / 4 ),
      ( double )( level - idx - 1 )
    ) );

    // calculate offset and adjust indirect block
    uint32_t offset = indirect_block / pow_result;
    indirect_block %= pow_result;
    // determine block number
    block_number = table[ offset ];
    // free cache again
    inode->fs->cache_block_release( cache, false );
  }
  // return offset
  return block_size * block_number;
}

/**
 * @fn bool ext_inode_read(ext_fs_t*, uint32_t, ext_inode_t*)
 * @brief Method to read an inode
 *
 * @param fs
 * @param inode_number
 * @param inode
 * @return
 */
bool ext_inode_read_inode( ext_fs_t* fs, uint32_t inode_number, ext_inode_t* inode ) {
  // get block and inode size
  uint32_t block_size = ext_superblock_block_size( fs->superblock );
  // calculate blockgroup number
  uint32_t blockgroup_number = ( inode_number - 1 ) /
    fs->superblock->s_inodes_per_group;
  // calculate inode offset
  uint32_t inode_offset = ext_superblock_inode_size( fs->superblock ) *
    ( ( inode_number - 1 ) % fs->superblock->s_inodes_per_group );
  // variable for block group
  ext_blockgroup_t blockgroup;
  // read blockgroup
  if ( ! ext_blockgroup_read( fs, blockgroup_number, &blockgroup ) ) {
    return false;
  }
  // read inode
  cache_block_t* cache = fs->cache_block_allocate(
    fs->handle,
    blockgroup.bg_inode_table + inode_offset / block_size,
    true
  );
  // handle error
  if ( ! cache ) {
    return false;
  }
  // populate inode
  inode->fs = fs;
  inode->inode_number = inode_number;
  inode->cache = cache;
  inode->inode = ( ext_inode_raw_t* )( cache->data + inode_offset % block_size );
  // return success
  return true;
}

/**
 * @fn bool ext_inode_write_inode(ext_fs_t*, uint32_t, ext_inode_t*)
 * @brief Helper to write inode to storage
 *
 * @param fs
 * @param inode_number
 * @param inode
 * @return
 *
 * @todo implement function
 */
bool ext_inode_write_inode(
  __unused ext_fs_t* fs,
  __unused uint32_t inode_number,
  __unused ext_inode_t* inode
) {
  return false;
}

/**
 * @fn bool ext_inode_release(ext_inode_t*)
 * @brief Method to release loaded inode
 *
 * @param inode
 * @return
 */
bool ext_inode_release_inode( ext_inode_t* inode ) {
  ext_fs_t* fs = inode->fs;
  // release cache block
  if ( ! fs->cache_block_release( inode->cache, false ) ) {
    return false;
  }
  // set pointers to NULL
  inode->cache = NULL;
  inode->inode = NULL;
  // return success
  return true;
}

/**
 * @fn uint32_t ext_inode_read_block(ext_inode_t*, uint32_t, uint8_t*, uint32_t)
 * @brief Method to read inode data block into buffer
 *
 * @param inode
 * @param block
 * @param buffer
 * @param count
 * @return
 */
uint32_t ext_inode_read_block(
  ext_inode_t* inode,
  uint32_t block,
  uint8_t* buffer,
  uint32_t count
) {
  // bunch of variables
  ext_fs_t* fs = inode->fs;
  uint32_t block_size = ext_superblock_block_size( fs->superblock );
  // loop until read count
  for ( uint32_t idx = 0; idx < count; idx++) {
    // get offset
    uint32_t offset = inode_block_get_offset(
      inode, block + idx, EXT_BLOCK_OFFSET_READ_ONLY );
    // zeros for sparse files
    if ( 0 == offset ) {
      memset( buffer + block_size * idx, 0, block_size );
      continue;
    }
    // read cache block
    cache_block_t* cache = fs->cache_block_allocate(
      fs->handle,
      offset / block_size,
      true
    );
    // handle error
    if ( ! cache ) {
      // free cache block again
      fs->cache_block_release( cache, false );
      // return current index
      return idx;
    }
    // copy over content
    memcpy( buffer + block_size * idx, cache->data, block_size );
    // free cache block again
    fs->cache_block_release( cache, false );
  }
  // return amount of read data
  return count;
}

/**
 * @fn uint32_t ext_inode_write_block(ext_inode_t*, uint32_t, uint8_t*)
 * @brief Write inode buffer to storage
 *
 * @param inode
 * @param block
 * @param buffer
 * @return
 *
 * @todo implement function
 */
uint32_t ext_inode_write_block(
  __unused ext_inode_t* inode,
  __unused uint32_t block,
  __unused uint8_t* buffer
) {
  return 0;
}

/**
 * @fn bool ext_inode_read_data(ext_inode_t*, uint32_t, uint32_t, uint8_t*)
 * @brief Method to read data of inode
 *
 * @param inode
 * @param start
 * @param length
 * @param buffer
 * @return
 */
bool ext_inode_read_data(
  ext_inode_t* inode,
  uint32_t start,
  uint32_t length,
  uint8_t* buffer
) {
  // get block size
  uint32_t block_size = ext_superblock_block_size( inode->fs->superblock );
  // calculate start and end block
  uint32_t start_block = start / block_size;
  uint32_t end_block = ( start + length - 1 ) / block_size;
  // calculate block count
  uint32_t block_count = end_block - start_block + 1;
  // allocate local buffer
  uint8_t* local_buffer = malloc( block_size );
  if ( ! local_buffer ) {
    return false;
  }
  // clear out allocated area
  memset( local_buffer, 0, block_size );
  // handle invalid length
  if ( 0 == length ) {
    free( local_buffer );
    return false;
  }

  // handle partial read of start block
  if ( start % block_size ) {
    uint32_t start_offset = start % block_size;
    // try to read inode block
    if ( 0 == ext_inode_read_block( inode, start_block, local_buffer, 1 ) ) {
      free( local_buffer );
      return false;
    }
    uint32_t byte = block_size - start_offset;
    // handle length smaller byte
    if ( length < byte ) {
      byte = length;
    }
    // copy content
    memcpy( buffer, local_buffer + start_offset, byte );
    // decrement block count
    --block_count;
    // handle finished
    if ( 0 == block_count ) {
      free( local_buffer );
      return true;
    }
    // adjust length, buffer and start block
    length -= byte;
    buffer += byte;
    start_block++;
  }

  // handle partial read of end block
  if ( length % block_size ) {
    uint32_t byte = length % block_size;
    // try to read inode block
    if ( 0 == ext_inode_read_block( inode, end_block, local_buffer, 1 ) ) {
      free( local_buffer );
      return false;
    }
    // copy content to end of buffer
    memcpy( buffer + length - byte, local_buffer, byte );
    // decrement block count
    --block_count;
    // handle finished
    if ( 0 == block_count ) {
      free( local_buffer );
      return true;
    }
    // adjust length and end block
    length -= byte;
    end_block--;
  }
  // free local buffer
  free( local_buffer );

  // read remaining blocks
  for ( uint32_t idx = 0; idx < block_count; idx++ ) {
    // read block
    uint32_t result = ext_inode_read_block(
      inode,
      start_block + idx,
      buffer + idx * block_size,
      block_count - idx
    );
    // handle done
    if ( 0 == result ) {
      return true;
    }
    // increment index by amount of read blocks
    idx += result - 1;
  }
  // return success
  return true;
}

/**
 * @fn bool ext_inode_write_data(ext_inode_t*, uint32_t, uint32_t, uint8_t*)
 * @brief Write inode data
 *
 * @param inode
 * @param start
 * @param length
 * @param buffer
 * @return
 *
 * @todo implement function
 */
bool ext_inode_write_data(
  __unused ext_inode_t* inode,
  __unused uint32_t start,
  __unused uint32_t length,
  __unused uint8_t* buffer
) {
  return false;
}

/**
 * @fn void ext_inode_dump(ext_inode_t*)
 * @brief Helper to dump inode data
 *
 * @param inode
 */
void ext_inode_dump( ext_inode_t* inode ) {
  EARLY_STARTUP_PRINT( "i_mode = %"PRIu16"\r\n", inode->inode->i_mode)
  EARLY_STARTUP_PRINT( "i_uid = %"PRIu16"\r\n", inode->inode->i_uid)
  EARLY_STARTUP_PRINT( "i_size = %"PRIu32"\r\n", inode->inode->i_size)
  EARLY_STARTUP_PRINT( "i_atime = %"PRIu32"\r\n", inode->inode->i_atime)
  EARLY_STARTUP_PRINT( "i_ctime = %"PRIu32"\r\n", inode->inode->i_ctime)
  EARLY_STARTUP_PRINT( "i_mtime = %"PRIu32"\r\n", inode->inode->i_mtime)
  EARLY_STARTUP_PRINT( "i_dtime = %"PRIu32"\r\n", inode->inode->i_dtime)
  EARLY_STARTUP_PRINT( "i_gid = %"PRIu16"\r\n", inode->inode->i_gid)
  EARLY_STARTUP_PRINT( "i_links_count = %"PRIu16"\r\n", inode->inode->i_links_count)
  EARLY_STARTUP_PRINT( "i_blocks = %"PRIu32"\r\n", inode->inode->i_blocks)
  EARLY_STARTUP_PRINT( "i_flags = %"PRIu32"\r\n", inode->inode->i_flags)
  EARLY_STARTUP_PRINT( "i_osd1 = %"PRIu32"\r\n", inode->inode->i_osd1)
  for ( uint32_t idx = 0; idx < 15; idx++ ) {
    EARLY_STARTUP_PRINT( "i_block[ %"PRIu32" ] = %"PRIu32"\r\n", idx, inode->inode->i_block[ idx ] )
  }
  EARLY_STARTUP_PRINT( "i_generation = %"PRIu32"\r\n", inode->inode->i_generation)
  EARLY_STARTUP_PRINT( "i_file_acl = %"PRIu32"\r\n", inode->inode->i_file_acl)
  EARLY_STARTUP_PRINT( "i_dir_acl = %"PRIu32"\r\n", inode->inode->i_dir_acl)
  EARLY_STARTUP_PRINT( "i_faddr = %"PRIu32"\r\n", inode->inode->i_faddr)
}
