
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

#if ! defined( ASSEMBLER_FILE )
  #include <stdint.h>
#endif

#if ! defined( __ARCH_ARM_MM_VIRT__ )
#define __ARCH_ARM_MM_VIRT__
  #if defined( ELF32 )
    // supported paging defines
    #define ID_MMFR0_VSMA_V6_PAGING 0x2
    #define ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS 0x3
    #define ID_MMFR0_VSMA_V7_PAGING_PXN 0x4
    #define ID_MMFR0_VSMA_V7_PAGING_LPAE 0x5

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
    #define LD_TTBCR_SIZE_TTBR_2G 0x1

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

    #if ! defined( ASSEMBLER_FILE )
      // methods
      void virt_setup_supported_modes( void );

      // supported modes
      uint32_t supported_modes;

      // helper macros
      #define SD_VIRTUAL_TO_TABLE( a ) ( a >> 20  )
      #define SD_VIRTUAL_TO_PAGE( a ) ( ( a >> 12 ) & 0xFF )

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
        uint64_t raw;
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
        uint32_t list[ 4096 ];
        sd_context_table_t table[ 4096 ];
        sd_context_section_t section[ 4096 ];
        sd_context_super_section_t super[ 4096 ];
      } sd_context_total_t;

      typedef union PACKED {
        uint32_t list[ 2048 ];
        sd_context_table_t table[ 2048 ];
        sd_context_section_t section[ 2048 ];
        sd_context_super_section_t super[ 2048 ];
      } sd_context_half_t;

      typedef struct {
        sd_page_small_t page[ 256 ];
      } sd_page_table_t;

      // FIXME: Add defines for long physical address extension

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
      } ld_context_block_level_1_t;

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
      } ld_context_block_level_2_t;

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
    #endif
  #endif
#endif
