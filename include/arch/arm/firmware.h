
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

#if ! defined( __ARCH_ARM_FIRMWARE__ )
#define __ARCH_ARM_FIRMWARE__

#include <stdint.h>

#if defined( ELF32 )
  typedef uint32_t firmware_register_t;
#elif defined( ELF64 )
  typedef uint64_t firmware_register_t;
#endif

typedef struct {
  firmware_register_t unused;
  firmware_register_t machine;
  firmware_register_t atag_fdt;
} firmware_t, *firmware_ptr_t;

extern firmware_t firmware_info;

#endif
