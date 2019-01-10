
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __KERNEL_ARCH_ARM_BARRIER__
#define __KERNEL_ARCH_ARM_BARRIER__

#include <stdint.h>

#if defined( __cplusplus )
extern "C" {
#endif

void barrier_data_mem( void );
void barrier_data_sync( void );
void barrier_flush_cache( void );

#if defined( __cplusplus )
}
#endif

#endif
