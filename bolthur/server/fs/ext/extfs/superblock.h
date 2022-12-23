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

#ifndef _EXTFS_SUPERBLOCK_H
#define _EXTFS_SUPERBLOCK_H

// superblock magic
#define EXT_SUPER_MAGIC 0xEF53
// state values
#define EXT_VALID_FS 1
#define EXT_ERROR_FS 2
// error values
#define EXT_ERRORS_CONTINUE 1
#define EXT_ERRORS_RO 2
#define EXT_ERRORS_PANIC 3
// creator os values
#define EXT_OS_LINUX 0
#define EXT_OS_HURD 1
#define EXT_OS_MASIX 2
#define EXT_OS_FREEBSD 3
#define EXT_OS_LITES 4
// rev level values
#define EXT_GOOD_OLD_REV 0
#define EXT_DYNAMIC_REV 1
// feature compatibility flags
#define EXT_FEATURE_COMPAT_DIR_PREALLOC 0x0001
#define EXT_FEATURE_COMPAT_IMAGIC_INODES 0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL 0x0004
#define EXT_FEATURE_COMPAT_EXT_ATTR 0x0008
#define EXT_FEATURE_COMPAT_RESIZE_INO 0x0010
#define EXT_FEATURE_COMPAT_DIR_INDEX 0x0020
// feature incompatibility flags
#define EXT_FEATURE_INCOMPAT_COMPRESSION 0x0001
#define EXT_FEATURE_INCOMPAT_FILETYPE 0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER 0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008
#define EXT_FEATURE_INCOMPAT_META_BG 0x0010
// feature read only compatibility flags
#define EXT_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT_FEATURE_RO_COMPAT_LARGE_FILE 0x0002
#define EXT_FEATURE_RO_COMPAT_BTREE_DIR 0x0004
// algo bitmap values
#define EXT_LZV1_ALG 0
#define EXT_LZRW3A_ALG 1
#define EXT_GZIP_ALG 2
#define EXT_BZIP2_ALG  3
#define EXT_LZO_ALG  4

#pragma pack(push, 1)

typedef struct {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;
  uint32_t s_first_data_block;
  uint32_t s_log_block_size;
  uint32_t s_log_frag_size;
  uint32_t s_blocks_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_mtime;
  uint32_t s_wtime;
  uint16_t s_mnt_count;
  uint16_t s_max_mnt_count;
  uint16_t s_magic;
  uint16_t s_state;
  uint16_t s_errors;
  uint16_t s_minor_rev_level;
  uint32_t s_lastcheck;
  uint32_t s_checkinterval;
  uint32_t s_creator_os;
  uint32_t s_rev_level;
  uint16_t s_def_resuid;
  uint16_t s_def_resgid;
  // extended stuff ( >= 1 )
  uint32_t s_first_ino;
  uint16_t s_inode_size;
  uint16_t s_block_group_nr;
  uint32_t s_feature_compat;
  uint32_t s_feature_incompat;
  uint32_t s_feature_ro_incompat;
  uint8_t s_uuid[ 16 ];
  char s_volume_name[ 16 ];
  char s_last_mounted[ 64 ];
  uint32_t s_algo_bitmap;
  // performance hints
  uint8_t s_prealloc_blocks;
  uint8_t s_prealloc_dir_blocks;
  uint16_t alignment;
  // Journaling support
  uint8_t s_journal_uuid[ 16 ];
  uint32_t s_journal_inum;
  uint32_t s_journal_dev;
  uint32_t s_last_orphan;
  // directory indexing support
  uint32_t s_hash_seed[ 4 ];
  uint8_t s_def_hash_version;
  uint8_t padding[ 3 ];
  // other options
  uint32_t s_default_mount_options;
  uint32_t s_first_meta_bg;
  uint32_t reserved[ 190 ];
} ext_superblock_t;

static_assert(
  1024 == sizeof( ext_superblock_t ),
  "invalid ext super block size!"
);

#pragma pack(pop)

#endif
