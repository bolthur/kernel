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

#include <stdint.h>

#if ! defined( _EXT2_H )
#define _EXT2_H

struct ext2_superblock {
};
typedef struct ext2_superblock ext2_superblock_t;
typedef struct ext2_superblock* ext2_superblock_ptr_t;

struct ext2_blockgroup {
  uint32_t block_bitmap;
  uint32_t inode_bitmap;
  uint32_t inode_table;
  uint16_t free_block_num;
  uint16_t free_inode_num;
  uint16_t directory_num;
  uint16_t padding;
  uint32_t unused[ 3 ];
};
typedef struct ext2_blockgroup ext2_blockgroup_t;
typedef struct ext2_blockgroup* ext2_blockgroup_ptr_t;

struct ext2_inode {
};
typedef struct ext2_inode ext2_inode_t;
typedef struct ext2_inode* ext2_inode_ptr_t;

struct ext2_directory_entry {
  uint32_t inode;
  uint16_t record_length;
  uint8_t name_length;
  uint8_t type;
  char name[];
};
typedef struct ext2_directory_entry ext2_directory_entry_t;
typedef struct ext2_directory_entry* ext2_directory_entry_ptr_t;

#endif
