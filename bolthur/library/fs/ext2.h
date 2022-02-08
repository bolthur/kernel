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

#define INODE_TYPE_FIFO 0x1000
#define INODE_TYPE_CHRDEV 0x2000
#define INODE_TYPE_DIR 0x4000
#define INODE_TYPE_BLKDEV 0x6000
#define INODE_TYPE_FILE 0x8000
#define INODE_TYPE_SYMLINK 0xA000
#define INODE_TYPE_SOCKET 0xC000

#define INODE_PERMISSION_HEX_OTHER_EXEC 0x1
#define INODE_PERMISSION_HEX_OTHER_WRITE 0x2
#define INODE_PERMISSION_HEX_OTHER_READ 0x4
#define INODE_PERMISSION_HEX_GROUP_EXEC 0x8
#define INODE_PERMISSION_HEX_GROUP_WRITE 0x10
#define INODE_PERMISSION_HEX_GROUP_READ 0x20
#define INODE_PERMISSION_HEX_USER_EXEC 0x40
#define INODE_PERMISSION_HEX_USER_WRITE 0x80
#define INODE_PERMISSION_HEX_USER_READ 0x100
#define INODE_PERMISSION_HEX_STICKY 0x200
#define INODE_PERMISSION_HEX_SET_GROUP 0x400
#define INODE_PERMISSION_HEX_SET_USER 0x800

#define INODE_PERMISSION_OCTAL_OTHER_EXEC 00001
#define INODE_PERMISSION_OCTAL_OTHER_WRITE 00002
#define INODE_PERMISSION_OCTAL_OTHER_READ 00004
#define INODE_PERMISSION_OCTAL_GROUP_EXEC 00010
#define INODE_PERMISSION_OCTAL_GROUP_WRITE 00020
#define INODE_PERMISSION_OCTAL_GROUP_READ 00040
#define INODE_PERMISSION_OCTAL_USER_EXEC 00100
#define INODE_PERMISSION_OCTAL_USER_WRITE 00200
#define INODE_PERMISSION_OCTAL_USER_READ 00400
#define INODE_PERMISSION_OCTAL_STICKY 01000
#define INODE_PERMISSION_OCTAL_SET_GROUP 02000
#define INODE_PERMISSION_OCTAL_SET_USER 04000

#define INODE_FLAG_SECURE_DELETION 0x1
#define INODE_FLAG_KEEP_DATA_COPY_AFTER_DELETION 0x2
#define INODE_FLAG_FILE_COMPRESSION 0x4
#define INODE_FLAG_SYNCHRONOUS_UPDATE 0x8
#define INODE_FLAG_IMMUTABLE_FILE 0x10
#define INODE_FLAG_APPEND_ONLY 0x20
#define INODE_FLAG_LAST_ACCESSED_NO_UPDATE 0x80
#define INODE_FLAG_HASH_INDEXED_DIR 0x10000
#define INODE_FLAG_AFS_DIR 0x20000
#define INODE_FLAG_JOURNAL_FILE_DATA 0x40000

struct ext2_inode {
  uint16_t type_permission;
  uint16_t uid_lower;
  uint32_t size;
  uint32_t access_time;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t deletion_time;
  uint16_t gid_lower;
  uint16_t link_count;
  uint32_t sector_count;
  uint32_t flags;
  uint32_t reserved1;
  uint32_t direct_block_pointer[ 15 ];
  uint32_t version;
  uint32_t file_acl;
  uint32_t dir_acl;
  uint32_t fragment_address;
  uint8_t fragment_number;
  uint8_t fragment_size;
  uint16_t reserved2;
  uint16_t uid_upper;
  uint16_t gid_upper;
  uint32_t reserved3;
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
