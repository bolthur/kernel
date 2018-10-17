
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBFDT__
#define __LIBFDT__

#include <stdint.h>

#define FDT_MAGIC 0xd00dfeed;

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP_NODE 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  uint32_t boot_cpuid_phys;
  uint32_t size_dt_strings;
  uint32_t size_dt_struct;
} fdt_header_t;

typedef struct {
  uint64_t address;
  uint64_t size;
} fdt_reserve_entry_t;

typedef struct {
  uint32_t token;
  uint32_t len;
  uint32_t nameoff;
  char data[];
} fdt_property_t;

#if defined( __cplusplus )
}
#endif

#endif
