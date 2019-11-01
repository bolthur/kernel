
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

#if ! defined( __KERNEL_BOOT_BSS__ )
#define __KERNEL_BOOT_BSS__

#include <stdint.h>

// type definition for bss fields
#if defined( ELF32 )
  typedef uint32_t bss_type_t;
#elif defined( ELF64 )
  typedef uint64_t bss_type_t;
#endif

// bss fields from linker script
bss_type_t __bss_start;
bss_type_t __bss_end;

#endif
