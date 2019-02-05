
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

#if ! defined( __BOOT_KERNEL_MM_VIRT__ )
#define __BOOT_KERNEL_MM_VIRT__

#include <stdbool.h>

#if defined( __cplusplus )
extern "C" {
#endif

void boot_virt_map_address( void*, void* );
void boot_virt_enable( void );
bool boot_virt_available( void );

#if defined( __cplusplus )
}
#endif

#endif
