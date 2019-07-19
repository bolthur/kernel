
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
#include <kernel/util.h>

#if ! defined( __ARCH_ARM_MM_VIRT_LONG__ )
#define __ARCH_ARM_MM_VIRT_LONG__

#if defined( ELF32 )
  // ttbcr defines
  #define LD_TTBCR_SIZE_TTBR_2G 0x1

  // helper macros
  #define LD_VIRTUAL_PMD_INDEX( a ) EXTRACT_BIT( a, 2, 31 )
  #define LD_VIRTUAL_TABLE_INDEX( a ) EXTRACT_BIT( a, 9, 29 )
  #define LD_VIRTUAL_PAGE_INDEX( a ) EXTRACT_BIT( a, 9, 20 )

  typedef union PACKED {
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

  typedef union PACKED {
    uint64_t raw;
    struct {
      uint32_t type : 1;
      uint32_t sbz_0 : 1;
      union {
        uint32_t data : 10;
        struct {
          uint32_t attribute_index : 3;
          uint32_t non_secure : 1;
          uint32_t access_permission : 2;
          uint32_t shared : 2;
          uint32_t access : 1;
          uint32_t not_global : 1;
        } attribute;
      } lower_block_attribute;
      uint32_t sbz_1 : 18;
      uint32_t output_address: 10;
      uint32_t sbz_2 : 12;
      union {
        uint32_t data : 12;
        struct {
          uint32_t continguous : 1;
          uint32_t privileged_execute_never : 1;
          uint32_t execute_never : 1;
          uint32_t ignored : 9;
        } attribute;
      } upper_block_attribute;
    } data;
  } ld_context_block_level1_t;

  typedef union PACKED {
    uint64_t raw;
    struct {
      uint32_t type : 1;
      uint32_t sbz_0 : 1;
      union {
        uint32_t data : 10;
        struct {
          uint32_t memory_attribute : 4;
          uint32_t access_permission : 2;
          uint32_t shared : 2;
          uint32_t access_flag : 1;
          uint32_t sbz : 1;
        } attribute;
      } lower_block_attribute;
      uint32_t sbz_1 : 9;
      uint32_t output_address: 19;
      uint32_t sbz_2 : 12;
      union {
        uint32_t data : 12;
        struct {
          uint32_t continguous : 1;
          uint32_t sbz : 1;
          uint32_t execute_never : 1;
          uint32_t ignored : 9;
        } attribute;
      } upper_block_attribute;
    } data;
  } ld_context_block_level2_t;

  typedef union PACKED {
    uint64_t raw;
    struct {
      uint32_t type : 2;
      uint32_t ignored_0 : 10;
      uint32_t next_level_table : 28;
      uint32_t sbz_0 : 12;
      uint32_t ignored_1 : 7;

      struct {
        uint32_t l2_sbz : 5;
        union {
          uint32_t privileged_execute_never : 1;
          uint32_t execute_never : 1;
          uint32_t access_permission : 2;
          uint32_t not_secure : 1;
        } l1_data;
      } table_attribute;
    } data;
  } ld_context_table_t;

  typedef union PACKED {
    uint64_t raw;
    struct {
      uint32_t type : 2;
      union {
        uint32_t data : 10;
        struct {
          uint32_t attribute_index : 3;
          uint32_t non_secure : 1;
          uint32_t access_permission : 2;
          uint32_t shared : 2;
          uint32_t access : 1;
          uint32_t not_global : 1;
        } attribute;
      } lower_block_attribute;
      uint32_t output_address : 28;
      uint32_t sbz_0 : 12;
      union {
        uint32_t data : 12;
        struct {
          uint32_t continguous : 1;
          uint32_t privileged_execute_never : 1;
          uint32_t execute_never : 1;
          uint32_t ignored : 9;
        } attribute;
      } upper_block_attribute;
    } data;
  } ld_context_page_t;

  typedef union PACKED {
    uint64_t raw[ 512 ];
    ld_context_table_t table[ 512 ];
  } ld_page_table_t;

  typedef union PACKED {
    uint64_t raw[ 512 ];
    ld_context_block_level2_t section[ 512 ];
    ld_context_table_t level[ 512 ];
  } ld_middle_page_directory;

  typedef union PACKED {
    uint64_t raw[ 512 ];
    ld_context_block_level1_t section[ 512 ];
    ld_context_table_t level[ 512 ];
  } ld_global_page_directory_t;
#endif

#endif
