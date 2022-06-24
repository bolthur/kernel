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
#include <assert.h>

#ifndef _EXT2_H
#define _EXT2_H

typedef union __packed {
  uint8_t plain[ 1024 ];
  struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_fragment_size;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t signature;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_version;
    uint32_t last_check_time;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t revision;
    uint16_t default_uid_for_reserved_block;
    uint16_t default_gid_for_reserved_block;

    // extended stuff ( >= 1 )
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t block_group_number;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_incompat;
    uint8_t uuid[ 16 ];
    char volume_name[ 16 ];
    char last_mount_path[ 64 ];
    uint32_t algorithm_bitmap;
    uint8_t preallocate_blocks;
    uint8_t preallocate_directory_blocks;
    uint16_t padding0;
    uint8_t journal_uuid[ 16 ];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t last_orphaned;
    uint32_t reserved[ 197 ];
  } data;
} ext2_superblock_t;

typedef union __packed {
  uint8_t plain[ 32 ];
  struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_block_num;
    uint16_t free_inode_num;
    uint16_t directory_num;
    uint16_t padding;
    uint8_t unused[ 12 ];
  } data;
} ext2_blockgroup_t;

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

typedef union __packed {
  uint8_t plain[ 72 ];
  struct {
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
  } data;
} ext2_inode_t;

typedef struct {
  uint32_t inode;
  uint16_t record_length;
  uint8_t name_length;
  uint8_t type;
  char name[];
} ext2_directory_entry_t;

#endif
