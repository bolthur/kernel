/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#if ! defined( _LIB_ATAG_H )
#define _LIB_ATAG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct atag_header atag_header_t;
typedef struct atag_header *atag_header_ptr_t;
typedef struct atag_core atag_core_t;
typedef struct atag_core *atag_core_ptr_t;
typedef struct atag_mem atag_mem_t;
typedef struct atag_mem *atag_mem_ptr_t;
typedef struct atag_initrd2 atag_initrd2_t;
typedef struct atag_initrd2 *atag_initrd2_ptr_t;
typedef struct atag_cmdline atag_cmdline_t;
typedef struct atag_cmdline *atag_cmdline_ptr_t;
typedef struct atag_ramdisk atag_ramdisk_t;
typedef struct atag_ramdisk *atag_ramdisk_ptr_t;
typedef struct atag_videotext atag_videotext_t;
typedef struct atag_videotext *atag_videotext_ptr_t;
typedef struct atag_serial atag_serial_t;
typedef struct atag_serial *atag_serial_ptr_t;
typedef struct atag_revision atag_revision_t;
typedef struct atag_revision *atag_revision_ptr_t;
typedef struct atag_videolfb atag_videolfb_t;
typedef struct atag_videolfb *atag_videolfb_ptr_t;
typedef struct atag atag_t;
typedef struct atag *atag_ptr_t;

typedef enum {
  ATAG_TAG_NONE = 0x00000000,
  ATAG_TAG_CORE = 0x54410001,
  ATAG_TAG_MEM = 0x54410002,
  ATAG_TAG_VIDEOTEXT = 0x54410003,
  ATAG_TAG_RAMDISK = 0x54420004,
  ATAG_TAG_INITRD2 = 0x54420005,
  ATAG_TAG_SERIAL = 0x54410006,
  ATAG_TAG_REVISION = 0x54410007,
  ATAG_TAG_VIDEOLFB = 0x54410008,
  ATAG_TAG_CMDLINE = 0x54410009,
} atag_tag_t;

struct atag_header {
  uint32_t size;
  uint32_t tag;
};

struct atag_core {
  uint32_t flag;
  uint32_t pagesize;
  uint32_t rootdev;
};

struct atag_mem {
  uint32_t size;
  uint32_t start;
};

struct atag_initrd2 {
  uint32_t start;
  uint32_t size;
};

struct atag_cmdline {
  char cmdline[ 1 ];
};

struct atag_ramdisk {
  uint32_t flag;
  uint32_t size;
  uint32_t start;
};

struct atag_videotext {
  uint8_t display_width;
  uint8_t display_height;
  uint16_t video_page;
  uint8_t video_mode;
  uint8_t video_cols;
  uint16_t video_ega_bx;
  uint8_t video_lines;
  uint8_t video_isvga;
  uint16_t video_points;
};

struct atag_serial {
  uint32_t low;
  uint32_t high;
};

struct atag_revision {
  uint32_t revision;
};

struct atag_videolfb {
  uint16_t lfb_width;
  uint16_t lfb_height;
  uint16_t lfb_depth;
  uint16_t lfb_linelength;
  uint32_t lfb_base;
  uint32_t lfb_size;
  uint8_t red_size;
  uint8_t red_pos;
  uint8_t green_size;
  uint8_t green_pos;
  uint8_t blue_size;
  uint8_t blue_pos;
  uint8_t rsvd_size;
  uint8_t rsvd_pos;
};

struct atag {
  atag_header_t header;

  union {
    atag_core_t core;
    atag_mem_t memory;
    atag_videotext_t videotext;
    atag_ramdisk_t ramdisk;
    atag_initrd2_t initrd;
    atag_serial_t serial;
    atag_revision_t revision;
    atag_videolfb_t videolfb;
    atag_cmdline_t cmdline;
  };
};

atag_ptr_t atag_next( atag_ptr_t );
bool atag_check( uintptr_t );
atag_ptr_t atag_find( atag_ptr_t, atag_tag_t );

#endif
