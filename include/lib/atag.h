
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if ! defined( __LIB_ATAG__ )
#define __LIB_ATAG__

#include <stdint.h>
#include <stddef.h>

enum tag {
  ATAG_TAG_NONE = 0x00000000,
  ATAG_TAG_CORE = 0x54410001,
  ATAG_TAG_MEM = 0x54410002,
  ATAG_TAG_INITRD2 = 0x54420005,
  ATAG_TAG_CMDLINE = 0x54410009,
};

typedef struct {
  uint32_t size;
  uint32_t start;
} atag_mem_t;

typedef struct {
  uint32_t start;
  uint32_t size;
} atag_initrd2_t;

typedef struct {
  char cmdline[ 1 ];
} atag_cmdline_t;

typedef struct {
  uint32_t tag_size;
  uint32_t tag_type;
  union {
    atag_mem_t memory;
    atag_initrd2_t initrd;
    atag_cmdline_t cmdline;
  };
} atag_t, *atag_ptr_t;

atag_ptr_t atag_next( atag_ptr_t );

#endif
