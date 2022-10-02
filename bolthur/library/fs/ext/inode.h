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

#ifndef _EXT_INODE_H
#define _EXT_INODE_H

#include <stdint.h>
#include <assert.h>

// reserved inodes
#define EXT_BAD_INO 1
#define EXT_ROOT_INO 2
#define EXT_ACL_IDX_INO 3
#define EXT_ACL_DATA_INO 4
#define EXT_BOOT_LOADER_INO 5
#define EXT_UNDEL_DIR_INO 6

// mode values
#define EXT_S_IFSOCK 0xC000
#define EXT_S_IFLNK  0xA000
#define EXT_S_IFREG  0x8000
#define EXT_S_IFBLK  0x6000
#define EXT_S_IFDIR  0x4000
#define EXT_S_IFCHR  0x2000
#define EXT_S_IFIFO  0x1000
// process execution user/group override
#define EXT_S_ISUID  0x0800
#define EXT_S_ISGID  0x0400
#define EXT_S_ISVTX  0x0200
// access rights
#define EXT_S_IRUSR  0x0100
#define EXT_S_IWUSR  0x0080
#define EXT_S_IXUSR  0x0040
#define EXT_S_IRGRP  0x0020
#define EXT_S_IWGRP  0x0010
#define EXT_S_IXGRP  0x0008
#define EXT_S_IROTH  0x0004
#define EXT_S_IWOTH  0x0002
#define EXT_S_IXOTH  0x0001

// i_flags
#define EXT_SECRM_FL 0x00000001
#define EXT_UNRM_FL  0x00000002
#define EXT_COMPR_FL 0x00000004
#define EXT_SYNC_FL  0x00000008
#define EXT_IMMUTABLE_FL 0x00000010
#define EXT_APPEND_FL  0x00000020
#define EXT_NODUMP_FL  0x00000040
#define EXT_NOATIME_FL 0x00000080
// Reserved for compression usage
#define EXT_DIRTY_FL 0x00000100
#define EXT_COMPRBLK_FL  0x00000200
#define EXT_NOCOMPR_FL 0x00000400
#define EXT_ECOMPR_FL  0x00000800
// End of compression flags
#define EXT_BTREE_FL 0x00001000
#define EXT_INDEX_FL 0x00001000
#define EXT_IMAGIC_FL  0x00002000
#define EXT3_JOURNAL_DATA_FL  0x00004000
#define EXT_RESERVED_FL  0x80000000

#pragma pack(push, 1)

typedef struct {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size;
  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks;
  uint32_t i_flags;
  uint32_t i_osd1;
  uint32_t i_block[ 15 ];
  uint32_t i_generation;
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  uint32_t i_faddr;
  uint8_t osd2[ 12 ];
} ext_inode_raw_t;

static_assert( 128 == sizeof( ext_inode_raw_t ), "invalid ext_inode_raw_t size!" );

#pragma pack(pop)

#endif
