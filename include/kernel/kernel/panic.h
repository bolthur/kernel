
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

#if ! defined( __KERNEL_PANIC__ )
#define __KERNEL_PANIC__

#include <stdint.h>

#define PANIC( msg ) panic( msg, __FILE__, __LINE__ );
#define ASSERT( b ) ( b ? (void)0 : panic_assert( __FILE__, __LINE__, #b ) );

void panic(const char* restrict, const char* restrict, uint32_t);
void panic_assert(const char* restrict, uint32_t, const char* restrict);

#endif
