/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#ifndef _INITRD_H
#define _INITRD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uintptr_t initrd_get_start_address( void );
void initrd_set_start_address( uintptr_t );
uintptr_t initrd_get_end_address( void );
size_t initrd_get_size( void );
void initrd_set_size( size_t );
bool initrd_exist( void );
void initrd_startup_init( void );

#endif
