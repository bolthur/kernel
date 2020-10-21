
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

#if ! defined( __CORE_VIDEO__ )
#define __CORE_VIDEO__

#include <stdint.h>

#define VIDEO_SCREEN_WIDTH 800
#define VIDEO_SCREEN_HEIGHT 600

void video_init( void );
void video_putc( uint8_t );

uintptr_t video_end_get( void );
uintptr_t video_base_get( void );
void video_base_set( uintptr_t );
uintptr_t video_virtual_destination( void );

#endif
