
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

#if defined( ELF32 )
  // page directory sizes
  #define VSMA_SHORT_PAGE_DIRECTORY_ALIGNMENT 0x4000
  #define VSMA_SHORT_PAGE_DIRECTORY_SIZE 0x2000

  #define VSMA_SHORT_PAGE_TABLE_ALIGNMENT 0x400
  #define VSMA_SHORT_PAGE_TABLE_SIZE 0x400
#endif

typedef enum {
  VMSA_SUPPORT_V6_PAGING = 0x2,
  VMSA_SUPPORT_V7_PAGING = 0x3,
  VSMA_SUPPORT_V7_PAGING_PXN = 0x4,
  VSMA_SUPPORT_V7_PAGING_LPAE = 0x5,
} virt_type_t;

#endif
