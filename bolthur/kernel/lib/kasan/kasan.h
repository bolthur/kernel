/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _LIB_KASAN_KASAN_H
#define _LIB_KASAN_KASAN_H

#include <stdint.h>

void __asan_store1_noabort( uintptr_t );
void __asan_store4_noabort( uintptr_t );
void __asan_store8_noabort( uintptr_t );
void __asan_storeN_noabort( uintptr_t );

void __asan_load1_noabort( uintptr_t );
void __asan_load2_noabort( uintptr_t );
void __asan_load4_noabort( uintptr_t );
void __asan_load8_noabort( uintptr_t );
void __asan_loadN_noabort( uintptr_t );

void __asan_handle_no_return( void );

#endif
