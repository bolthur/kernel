
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

#ifndef __KERNEL_SERIAL__
#define __KERNEL_SERIAL__

#include <stdint.h>

#if defined( __cplusplus )
extern "C" {
#endif

void serial_init( void );
void serial_putc( uint8_t c );
uint8_t serial_getc( void );
void serial_flush( void );

#if defined( __cplusplus )
}
#endif

#endif
