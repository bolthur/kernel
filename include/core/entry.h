
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

#if ! defined( __CORE_ENTRY__ )
#define __CORE_ENTRY__

#if defined( IS_HIGHER_HALF )
  #if defined( ELF32 )
    #define KERNEL_OFFSET 0xC0000000
  #elif defined( ELF64 )
    #define KERNEL_OFFSET 0xffffffff80000000
  #endif
#else
  #define KERNEL_OFFSET 0
#endif

#if !defined( ASSEMBLER_FILE )
  #include <stdint.h>

  #define PHYS_2_VIRT( a ) ( ( uintptr_t )a + KERNEL_OFFSET )
  #define VIRT_2_PHYS( a ) ( ( uintptr_t )a - KERNEL_OFFSET )

  extern uintptr_t __kernel_start;
  extern uintptr_t __kernel_end;
#endif

#endif
