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

#if ! defined( _ARCH_ARM_MM_VIRT_LONG_H )
#define _ARCH_ARM_MM_VIRT_LONG_H

#include <stdint.h>

#if defined( ELF32 )
  // entry types
  #define LD_TYPE_INVALID 0
  #define LD_TYPE_SECTION 0x1
  #define LD_TYPE_TABLE 0x3
  #define LD_TYPE_PAGE 0x3

  // access permissions
  #define LD_AP_RW_PRIVILEGED 0x0
  #define LD_AP_RW_ANY 0x1
  #define LD_AP_RO_PRIVILEGED 0x2
  #define LD_AP_RO_ANY 0x3

  // helper macros
  #define LD_VIRTUAL_PMD_INDEX( a ) ( ( a & 0xC0000000 ) >> 30 )
  #define LD_VIRTUAL_TABLE_INDEX( a ) ( ( a & 0x3FE00000 ) >> 21 )
  #define LD_VIRTUAL_PAGE_INDEX( a ) ( ( a & 0x1FF000 ) >> 12 )

  #define LD_PHYSICAL_SECTION_L1_ADDRESS( a ) ( ( uint64_t )a & 0x7FC0000000 )
  #define LD_PHYSICAL_SECTION_L2_ADDRESS( a ) ( ( uint64_t )a & 0x7FFFE00000 )
  #define LD_PHYSICAL_TABLE_ADDRESS( a ) ( ( uint64_t )a & 0xFFFFFFF000 )
  #define LD_PHYSICAL_PAGE_ADDRESS( a ) ( ( uint64_t )a & 0xFFFFFFF000 )

  typedef union __packed {
    uint32_t raw;
    struct {
      uint32_t ttbr0_size : 3;
      uint32_t sbz_0 : 4;
      uint32_t ttbr0_disable_table_walk : 1;
      uint32_t ttbr0_inner_cachability : 2;
      uint32_t ttbr0_outer_cachability : 2;
      uint32_t ttbr0_shareability : 2;
      uint32_t sbz_1 : 2;
      uint32_t ttbr1_size : 3;
      uint32_t sbz_2 : 3;
      uint32_t ttbr0_ttbr1_asid : 1;
      uint32_t ttbr1_disable_table_walk : 1;
      uint32_t ttbr1_inner_cachability : 2;
      uint32_t ttbr1_outer_cachability : 2;
      uint32_t ttbr1_shareability : 2;
      uint32_t imp : 1;
      uint32_t large_physical_address_extension : 1;
    } data;
  } ld_ttbcr_t;

  typedef union __packed {
    uint64_t raw;
    struct {
      uint64_t type : 1;
      uint64_t sbz_0 : 1;

      uint64_t lower_attr_attribute_index : 3;
      uint64_t lower_attr_non_secure : 1;
      uint64_t lower_attr_access_permission : 2;
      uint64_t lower_attr_shared : 2;
      uint64_t lower_attr_access : 1;
      uint64_t lower_attr_not_global : 1;

      uint64_t sbz_1 : 18;
      uint64_t output_address: 10;
      uint64_t sbz_2 : 12;

      uint64_t upper_attr_continguous : 1;
      uint64_t upper_attr_privileged_execute_never : 1;
      uint64_t upper_attr_execute_never : 1;
      uint64_t upper_attr_ignored : 9;
    } data;
  } ld_context_block_level1_t;

  typedef union __packed {
    uint64_t raw;
    struct {
      uint64_t type : 1;
      uint64_t sbz_0 : 1;

      uint64_t lower_attr_attribute_index : 3;
      uint64_t lower_attr_non_secure : 1;
      uint64_t lower_attr_access_permission : 2;
      uint64_t lower_attr_shared : 2;
      uint64_t lower_attr_access : 1;
      uint64_t lower_attr_not_global : 1;

      uint64_t sbz_1 : 9;
      uint64_t output_address: 19;
      uint64_t sbz_2 : 12;

      uint64_t upper_attr_continguous : 1;
      uint64_t upper_attr_sbz0 : 1;
      uint64_t upper_attr_execute_never : 1;
      uint64_t upper_attr_software_usage : 4;
      uint64_t upper_attr_ignored : 5;
    } data;
  } ld_context_block_level2_t;

  typedef union __packed {
    uint64_t raw;
    struct {
      uint64_t type : 2;
      uint64_t ignored_0 : 10;
      uint64_t next_level_table : 28;
      uint64_t sbz_0 : 12;
      uint64_t ignored_1 : 7;
      uint64_t attr_pxn_table : 1;
      uint64_t attr_xn_table : 1;
      uint64_t attr_ap_table : 2;
      uint64_t attr_ns_table : 1;
    } data;
  } ld_context_table_level1_t;

  typedef union __packed {
    uint64_t raw;
    struct {
      uint64_t type : 2;
      uint64_t ignored_0 : 10;
      uint64_t next_level_table : 28;
      uint64_t sbz_0 : 12;
      uint64_t ignored_1 : 7;
      uint64_t sbz_1 : 5;
    } data;
  } ld_context_table_level2_t;

  typedef union __packed {
    uint64_t raw;
    struct {
      uint64_t type : 2;

      uint64_t lower_attr_memory_attribute : 4;
      uint64_t lower_attr_access_permission : 2;
      uint64_t lower_attr_shared : 2;
      uint64_t lower_attr_access : 1;
      uint64_t lower_attr_sbz0 : 1;

      uint64_t output_address : 28;
      uint64_t sbz_0 : 12;

      uint64_t upper_attr_continguous : 1;
      uint64_t upper_attr_sbz0 : 1;
      uint64_t upper_attr_execute_never : 1;
      uint64_t upper_attr_software_usage : 4;
      uint64_t upper_attr_system_mmu_usage : 5;
    } data;
  } ld_context_page_t;

  typedef union __packed {
    uint64_t raw[ 512 ];
    ld_context_page_t page[ 512 ];
  } ld_page_table_t;

  typedef union __packed {
    uint64_t raw[ 512 ];
    ld_context_block_level2_t section[ 512 ];
    ld_context_table_level2_t table[ 512 ];
  } ld_middle_page_directory;

  typedef union __packed {
    uint64_t raw[ 512 ];
    ld_context_block_level1_t section[ 512 ];
    ld_context_table_level1_t table[ 512 ];
  } ld_global_page_directory_t;
#endif

#endif
