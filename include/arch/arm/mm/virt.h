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

#include <stdint.h>

#if ! defined( __ARCH_ARM_MM_VIRT__ )
#define __ARCH_ARM_MM_VIRT__

#if defined( ELF32 )
  // supported paging defines
  #define ID_MMFR0_VSMA_V6_PAGING 0x2
  #define ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS 0x3
  #define ID_MMFR0_VSMA_V7_PAGING_PXN 0x4
  #define ID_MMFR0_VSMA_V7_PAGING_LPAE 0x5

  // methods
  void virt_setup_supported_modes( void );
  void virt_startup_setup_supported_modes( void );

  // supported modes
  extern uint32_t virt_supported_mode;
  extern uint32_t virt_startup_supported_mode;
#endif

uintptr_t virt_prefetch_fault_address( void );
uintptr_t virt_prefetch_status( void );
uintptr_t virt_data_fault_address( void );
uintptr_t virt_data_status( void );

#endif
