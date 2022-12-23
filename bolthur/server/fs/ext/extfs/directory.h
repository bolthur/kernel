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

#ifndef _EXTFS_DIRECTORY_H
#define _EXTFS_DIRECTORY_H

#pragma pack(push, 1)

// file_type
#define EXT_FT_UNKNOWN 0
#define EXT_FT_REG_FILE 1
#define EXT_FT_DIR 2
#define EXT_FT_CHRDEV 3
#define EXT_FT_BLKDEV 4
#define EXT_FT_FIFO 5
#define EXT_FT_SOCK 6
#define EXT_FT_SYMLINK 7

typedef struct {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[];
} ext_directory_entry_t;

static_assert(
  8 == sizeof( ext_directory_entry_t ),
  "invalid ext_directory_indexed_root_t size!"
);

// hash version
#define DX_HASH_LEGACY 0
#define DX_HASH_HALF_MD4 1
#define DX_HASH_TEA 2

typedef struct {
  uint32_t reserved_sbz;
  uint8_t hash_version;
  uint8_t info_length;
  uint8_t indirect_levels;
  uint8_t unused;
} ext_directory_indexed_root_t;

static_assert(
  8 == sizeof( ext_directory_indexed_root_t ),
  "invalid ext_directory_indexed_root_t size!"
);

#pragma pack(pop)

#endif
