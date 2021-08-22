/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <stddef.h>
#include <unistd.h>
#include "../libframebuffer.h"

#if ! defined( __OUTPUT__ )
#define __OUTPUT__

#define OUTPUT_FONT_WIDTH 12
#define OUTPUT_FONT_HEIGHT 12

extern framebuffer_resolution_t resolution_data;

void output_init( void );
void output_handle_out( pid_t, size_t );
void output_handle_err( pid_t, size_t );

#endif
