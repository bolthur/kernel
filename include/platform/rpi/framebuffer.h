
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

#if ! defined( __KERNEL_VENDOR_RPI_FRAMEBUFFER__ )
#define __KERNEL_VENDOR_RPI_FRAMEBUFFER__

#include <stdint.h>

#define FRAMEBUFFER_SCREEN_WIDTH 800
#define FRAMEBUFFER_SCREEN_HEIGHT 600
#define FRAMEBUFFER_SCREEN_DEPTH 16

void framebuffer_init( void );
void framebuffer_putc( uint8_t );

uintptr_t framebuffer_end_get( void );
uintptr_t framebuffer_base_get( void );
void framebuffer_base_set( uintptr_t );

#endif
