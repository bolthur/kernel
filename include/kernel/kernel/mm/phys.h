
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

#include <stdint.h>
#include <stddef.h>

#if ! defined( __KERNEL_MM_PHYS__ )
#define __KERNEL_MM_PHYS__

extern uintptr_t *phys_bitmap;
extern size_t phys_bitmap_length;
extern uintptr_t placement_address;
extern uintptr_t __kernel_start;
extern uintptr_t __kernel_end;

void phys_init( void );

void phys_mark_page_used( void* );
void phys_mark_page_free( void* );
void* phys_find_free_range( size_t, size_t );
void phys_free_range( void*, size_t );

#endif
