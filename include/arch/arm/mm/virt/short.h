
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

#include <stdint.h>

#if ! defined( __ARCH_ARM_MM_VIRT_SHORT__ )
#define __ARCH_ARM_MM_VIRT_SHORT__

#if defined( ELF32 )
  // memory access permissions
  #define SD_MAC_APX0_NO_ACCESS 0x0
  #define SD_MAC_APX0_PRIVILEGED_RW 0x1
  #define SD_MAC_APX0_USER_RO 0x2
  #define SD_MAC_APX0_FULL_RW 0x3
  #define SD_MAC_APX1_RESERVED 0x0
  #define SD_MAC_APX1_PRIVILEGED_RO 0x1
  #define SD_MAC_APX1_USER_RO 0x2
  #define SD_MAC_APX1_FULL_RO 0x3

  // domain access
  #define SD_DOMAIN_NO_ACCESS 0x0
  #define SD_DOMAIN_CLIENT 0x1
  #define SD_DOMAIN_RESERVED 0x2
  #define SD_DOMAIN_MANAGER 0x3

  // ttbcr defines
  #define SD_TTBCR_N_TTBR0_4G 0x0
  #define SD_TTBCR_N_TTBR0_2G 0x1
  #define SD_TTBCR_N_TTBR0_1G 0x2
  #define SD_TTBCR_N_TTBR0_512M 0x3
  #define SD_TTBCR_N_TTBR0_256M 0x4
  #define SD_TTBCR_N_TTBR0_128M 0x5
  #define SD_TTBCR_N_TTBR0_64M 0x6
  #define SD_TTBCR_N_TTBR0_32M 0x7

  // ttbr0 sizes
  #define SD_TTBR_SIZE_4G 0x4000
  #define SD_TTBR_SIZE_2G 0x2000
  #define SD_TTBR_SIZE_1G 0x1000
  #define SD_TTBR_SIZE_512m 0x800
  #define SD_TTBR_SIZE_256M 0x400
  #define SD_TTBR_SIZE_128M 0x200
  #define SD_TTBR_SIZE_64M 0x100
  #define SD_TTBR_SIZE_32M 0x80

  // ttbr0 alignments
  #define SD_TTBR_ALIGNMENT_4G 0x4000
  #define SD_TTBR_ALIGNMENT_2G 0x2000
  #define SD_TTBR_ALIGNMENT_1G 0x1000
  #define SD_TTBR_ALIGNMENT_512m 0x800
  #define SD_TTBR_ALIGNMENT_256M 0x400
  #define SD_TTBR_ALIGNMENT_128M 0x200
  #define SD_TTBR_ALIGNMENT_64M 0x100
  #define SD_TTBR_ALIGNMENT_32M 0x80

  // ttbr1 start address depending on ttbr0 size
  #define SD_TTBR1_START_TTBR0_2G 0x80000000
  #define SD_TTBR1_START_TTBR0_1G 0x40000000
  #define SD_TTBR1_START_TTBR0_512M 0x20000000
  #define SD_TTBR1_START_TTBR0_256M 0x10000000
  #define SD_TTBR1_START_TTBR0_128M 0x08000000
  #define SD_TTBR1_START_TTBR0_64M 0x04000000
  #define SD_TTBR1_START_TTBR0_32M 0x02000000

  // first level types
  #define SD_TTBR_TYPE_INVALID 0
  #define SD_TTBR_TYPE_PAGE_TABLE 1
  #define SD_TTBR_TYPE_SECTION 1
  #define SD_TTBR_TYPE_SECTION_PXN 1

  // page table sizes
  #define SD_TBL_SIZE 0x400
  #define SD_PAGE_SIZE 0x1000

  // second level table
  #define SD_TBL_INVALID 0
  #define SD_TBL_LARGE_PAGE 0
  #define SD_TBL_SMALL_PAGE 1

  // helper macros
  #define SD_VIRTUAL_TABLE_INDEX( a ) ( a >> 20  )
  #define SD_VIRTUAL_PAGE_INDEX( a ) ( ( a >> 12 ) & 0xFF )

  typedef union PACKED {
    uint32_t raw;
    struct {
      uint32_t ttbr_split : 3;
      uint32_t sbz_0 : 1;
      union {
        uint32_t sbz_1 : 2;
        struct {
          uint32_t walk_0 : 1;
          uint32_t walk_1 : 1;
        } table;
      } disable;
      uint32_t sbz_2 : 26;
      uint32_t large_physical_address_extension : 1;
    } data;
  } sd_ttbcr_t;

  typedef union PACKED {
    uint32_t raw;
    struct {
      uint32_t execute_never : 1;
      uint32_t type: 1;
      uint32_t bufferable : 1;
      uint32_t cacheable : 1;
      uint32_t access_permision_0 : 2;
      uint32_t tex : 3;
      uint32_t access_permision_1 : 1;
      uint32_t shareable : 1;
      uint32_t not_global : 1;
      uint32_t frame : 20;
    } data;
  } sd_page_small_t;

  typedef union PACKED {
    uint32_t raw;
    struct {
      uint32_t type: 1;
      uint32_t sbz_0 : 1;
      uint32_t bufferable : 1;
      uint32_t cacheable : 1;
      uint32_t access_permision_0 : 2;
      uint32_t sbz_1 : 3;
      uint32_t access_permision_1 : 1;
      uint32_t shareable : 1;
      uint32_t not_global : 1;
      uint32_t tex : 3;
      uint32_t execute_never : 1;
      uint32_t frame : 16;
    } data;
  } sd_page_large_t;

  typedef union PACKED {
    uint32_t raw;
    struct {
      uint32_t type : 2;
      uint32_t privileged_execute_never : 1;
      uint32_t non_secure : 1;
      uint32_t sbz : 1;
      uint32_t domain : 4;
      uint32_t imp : 1;
      uint32_t frame : 22;
    } data;
  } sd_context_table_t;

  typedef union  PACKED {
    uint32_t raw;
    struct {
      uint32_t privileged_execute_never : 1;
      uint32_t type : 1;
      uint32_t bufferable : 1;
      uint32_t cacheable : 1;
      uint32_t execute_never : 1;
      uint32_t domain : 4;
      uint32_t imp : 1;
      uint32_t access_permision_0 : 2;
      uint32_t tex : 3;
      uint32_t access_permision_1 : 1;
      uint32_t shareable : 1;
      uint32_t not_global : 1;
      uint32_t sbz : 1;
      uint32_t non_secure : 1;
      uint32_t frame: 12;
    } data;
  } sd_context_section_t;

  typedef union  PACKED {
    uint32_t raw;
    struct {
      uint32_t privileged_execute_never : 1;
      uint32_t type : 1;
      uint32_t bufferable : 1;
      uint32_t cacheable : 1;
      uint32_t execute_never : 1;
      uint32_t base_bit_36_39 : 4;
      uint32_t imp : 1;
      uint32_t access_permision_0 : 2;
      uint32_t tex : 3;
      uint32_t access_permision_1 : 1;
      uint32_t shareable : 1;
      uint32_t not_global : 1;
      uint32_t sb1 : 1;
      uint32_t non_secure : 1;
      uint32_t base_bit_32_35 : 4;
      uint32_t base_bit_24_31 : 8;
    } data;
  } sd_context_super_section_t;

  typedef union PACKED {
    uint32_t raw[ 4096 ];
    sd_context_table_t table[ 4096 ];
    sd_context_section_t section[ 4096 ];
    sd_context_super_section_t super[ 4096 ];
  } sd_context_total_t;

  typedef union PACKED {
    uint32_t raw[ 2048 ];
    sd_context_table_t table[ 2048 ];
    sd_context_section_t section[ 2048 ];
    sd_context_super_section_t super[ 2048 ];
  } sd_context_half_t;

  typedef struct {
    sd_page_small_t page[ 256 ];
  } sd_page_table_t;
#endif

#endif
