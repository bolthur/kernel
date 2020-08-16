
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#if ! defined( __LIB_FDT__ )
#define __LIB_FDT__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <endian.h>

typedef struct {
  uint32_t magic;
  uint32_t totalsize;
  uint32_t off_dt_struct;
  uint32_t off_dt_strings;
  uint32_t off_mem_rsvmap;
  uint32_t version;
  uint32_t last_comp_version;
  // version 2 ongoing
  uint32_t boot_cpuid_phys;
  // version 3 ongoing
  uint32_t size_dt_strings;
  // version 17 ongoing
  uint32_t size_dt_struct;
} fdt_header_t, *fdt_header_ptr_t;

typedef struct {
  uint64_t address;
  uint64_t size;
} fdt_reserved_entry_t, *fdt_reserved_entry_ptr_t;

typedef struct {
  uint32_t tag;
  char name[];
} fdt_node_header_t, *fdt_node_header_ptr_t;

typedef struct {
  uint32_t tag;
  uint32_t len;
  uint32_t nameoff;
  char data[];
} fdt_property_t, *fdt_property_ptr_t;

#define FDT_MAGIC 0xd00dfeed
#define FDT_TAGSIZE sizeof( uint32_t )

#define FDT_NODE_BEGIN 0x1
#define FDT_NODE_END  0x2
#define FDT_PROP 0x3
#define FDT_NOP 0x4
#define FDT_END 0x9

#define FDT_V1_SIZE ( 7 * sizeof( uint32_t ) )
#define FDT_V2_SIZE ( FDT_V1_SIZE + sizeof( uint32_t ) )
#define FDT_V3_SIZE ( FDT_V2_SIZE + sizeof( uint32_t ) )
#define FDT_V16_SIZE FDT_V3_SIZE
#define FDT_V17_SIZE ( FDT_V16_SIZE + sizeof( uint32_t ) )

#define FDT_FIRST_SUPPORTED_VERSION 0x02
#define FDT_LAST_SUPPORTED_VERSION 0x11

#define fdt_reserved_get_attribute( address, field ) ( be64toh( ( ( fdt_reserved_entry_ptr_t )address )->field ) )
#define fdt_reserved_get_address( address ) ( fdt_reserved_get_attribute( address, len ) )
#define fdt_reserved_get_size( address ) ( fdt_reserved_get_attribute( address, nameoff ) )

#define fdt_property_get_attribute( address, field ) ( be32toh( ( ( fdt_property_ptr_t )address )->field ) )
#define fdt_property_get_len( address ) ( fdt_property_get_attribute( address, len ) )
#define fdt_property_get_nameoff( address ) ( fdt_property_get_attribute( address, nameoff ) )

#define fdt_header_get_attribute( address, field ) ( be32toh( ( ( fdt_header_ptr_t )address )->field ) )
#define fdt_header_get_magic( address ) ( fdt_header_get_attribute( address, magic ) )
#define fdt_header_get_totalsize( address ) ( fdt_header_get_attribute( address, totalsize ) )
#define fdt_header_get_off_dt_struct( address ) ( fdt_header_get_attribute( address, off_dt_struct ) )
#define fdt_header_get_off_dt_strings( address ) ( fdt_header_get_attribute( address, off_dt_strings ) )
#define fdt_header_get_off_mem_rsvmap( address ) ( fdt_header_get_attribute( address, off_mem_rsvmap ) )
#define fdt_header_get_version( address ) ( fdt_header_get_attribute( address, version ) )
#define fdt_header_get_last_comp_version( address ) ( fdt_header_get_attribute( address, last_comp_version ) )
#define fdt_header_get_boot_cpuid_phys( address ) ( fdt_header_get_attribute( address, boot_cpuid_phys ) )
#define fdt_header_get_size_dt_strings( address ) ( fdt_header_get_attribute( address, size_dt_strings ) )
#define fdt_header_get_size_dt_struct( address ) ( fdt_header_get_attribute( address, size_dt_struct ) )

bool fdt_check_header( uintptr_t );
bool fdt_check_offset(  uint32_t, uint32_t, uint32_t);
bool fdt_check_block( uint32_t, uint32_t, uint32_t, uint32_t );
size_t fdt_header_size( uintptr_t );

#endif
