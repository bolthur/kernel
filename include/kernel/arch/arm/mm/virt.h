
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#if ! defined( __KERNEL_ARCH_ARM_MM_VIRT__ )
#define __KERNEL_ARCH_ARM_MM_VIRT__
  /////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////// OLD ////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  // page directory sizes
  #define VSMA_SHORT_PAGE_DIRECTORY_ALIGNMENT 0x4000
  #define VSMA_SHORT_PAGE_TABLE_SIZE 0x400
  #define TTBR_L1_IS_PAGETABLE 0x1
  #define TTBR_L1_IS_SECTION 0x2




  /////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////// NEW ////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  #if defined( ELF32 )
    // helper macros
    #define SD_VIRTUAL_TO_TABLE( a ) ( ( uint32_t ) a >> 20  )
    #define SD_VIRTUAL_TO_PAGE( a ) ( ( ( uint32_t ) a >> 12 ) & 0xFF )

    // short descriptor format defines
    #define ID_MMFR0_VSMA_SUPPORT_V6_PAGING 0x2
    #define ID_MMFR0_VSMA_SUPPORT_V7_PAGING_REMAP_ACCESS 0x3
    #define ID_MMFR0_VSMA_SUPPORT_V7_PAGING_PXN 0x4
    #define ID_MMFR0_VSMA_SUPPORT_V7_PAGING_LPAE 0x5

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
    #define SD_TTBCR_N_TTBR0_4G ( 0x0 << 0 )
    #define SD_TTBCR_N_TTBR0_2G ( 0x1 << 0 )
    #define SD_TTBCR_N_TTBR0_1G ( 0x2 << 0 )
    #define SD_TTBCR_N_TTBR0_512M ( 0x3 << 0 )
    #define SD_TTBCR_N_TTBR0_256M ( 0x4 << 0 )
    #define SD_TTBCR_N_TTBR0_128M ( 0x5 << 0 )
    #define SD_TTBCR_N_TTBR0_64M ( 0x6 << 0 )
    #define SD_TTBCR_N_TTBR0_32M ( 0x7 << 0 )
    #define SD_TTBCR_TABLE_WALK_TTBR0 ( 0x1 << 4 )
    #define SD_TTBCR_TABLE_WALK_TTBR1 ( 0x1 << 5 )
    #define SD_TTBCR_EAE ( 0x1 << 31 )

    #define foo 0xfffff
    // ttbr0 sizes
    #define SD_TTBR0_SIZE_4G 0x4000
    #define SD_TTBR0_SIZE_2G 0x2000
    #define SD_TTBR0_SIZE_1G 0x1000
    #define SD_TTBR0_SIZE_512m 0x800
    #define SD_TTBR0_SIZE_256M 0x400
    #define SD_TTBR0_SIZE_128M 0x200
    #define SD_TTBR0_SIZE_64M 0x100
    #define SD_TTBR0_SIZE_32M 0x80

    // ttbr0 alignments
    #define SD_TTBR0_ALIGNMENT_4G 0x4000
    #define SD_TTBR0_ALIGNMENT_2G 0x2000
    #define SD_TTBR0_ALIGNMENT_1G 0x1000
    #define SD_TTBR0_ALIGNMENT_512m 0x800
    #define SD_TTBR0_ALIGNMENT_256M 0x400
    #define SD_TTBR0_ALIGNMENT_128M 0x200
    #define SD_TTBR0_ALIGNMENT_64M 0x100
    #define SD_TTBR0_ALIGNMENT_32M 0x80

    // ttbr1 sizes
    #define SD_TTBR1_SIZE 0x4000

    // ttbr1 alignments
    #define SD_TTBR0_ALIGNMENT 0x4000

    // ttbr1 start address depending on ttbr0 size
    #define SD_TTBR1_START_TTBR0_2G 0x80000000
    #define SD_TTBR1_START_TTBR0_1G 0x40000000
    #define SD_TTBR1_START_TTBR0_512M 0x20000000
    #define SD_TTBR1_START_TTBR0_256M 0x10000000
    #define SD_TTBR1_START_TTBR0_128M 0x08000000
    #define SD_TTBR1_START_TTBR0_64M 0x04000000
    #define SD_TTBR1_START_TTBR0_32M 0x02000000

    // first level table types
    #define SD_TTBR0_TYPE_INVALID 0x0
    #define SD_TTBR0_TYPE_PAGE_TABLE 0x1
    #define SD_TTBR0_TYPE_SECTION 0x2
    #define SD_TTBR0_TYPE_SECTION_PXN 0x3

    // first level memory access permissions
    #define SD_SECTION_APX0_NO_ACCESS ( MAC_SD_APX0_NO_ACCESS << 4 )
    #define SD_SECTION_APX0_PRIVILEGED_RW ( MAC_SD_APX0_PRIVILEGED_RW << 4 )
    #define SD_SECTION_APX0_PRIVILEGED_RW_USER_R ( MAC_SD_APX0_USER_RO << 4 )
    #define SD_SECTION_APX0_FULL_RW ( MAC_SD_APX0_FULL_RW << 4 )
    #define SD_SECTION_APX1_RESERVED ( MAC_SD_APX1_RESERVED << 4 )
    #define SD_SECTION_APX1_PRIVILEGED_RO ( MAC_SD_APX1_PRIVILEGED_RO << 4 )
    #define SD_SECTION_APX1_AFULL_RO_DEPRECATED ( MAC_SD_APX1_USER_RO << 4 )
    #define SD_SECTION_APX1_FULL_RO ( MAC_SD_APX1_ << 4 )

    // second level table
    #define SD_TBL_INVALID 0x0
    #define SD_TBL_LARGE_PAGE 0x1
    #define SD_TBL_SMALL_PAGE 0x2

    // second level memory access permissions
    #define SD_TLB_APX0_NO_ACCESS ( MAC_SD_APX0_NO_ACCESS << 4 )
    #define SD_TLB_APX0_PRIVILEGED_RW ( MAC_SD_APX0_PRIVILEGED_RW << 4 )
    #define SD_TLB_APX0_PRIVILEGED_RW_USER_R ( MAC_SD_APX0_USER_RO << 4 )
    #define SD_TLB_APX0_FULL_RW ( MAC_SD_APX0_FULL_RW << 4 )
    #define SD_TLB_APX1_RESERVED ( MAC_SD_APX1_RESERVED << 4 )
    #define SD_TLB_APX1_PRIVILEGED_RO ( MAC_SD_APX1_PRIVILEGED_RO << 4 )
    #define SD_TLB_APX1_AFULL_RO_DEPRECATED ( MAC_SD_APX1_USER_RO << 4 )
    #define SD_TLB_APX1_FULL_RO ( MAC_SD_APX1_ << 4 )
  #endif
#endif
