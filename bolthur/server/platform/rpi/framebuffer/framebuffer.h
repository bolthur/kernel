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

#if ! defined( __FRAMEBUFFER__ )
#define __FRAMEBUFFER__

#define FRAMEBUFFER_SCREEN_WIDTH 1024
#define FRAMEBUFFER_SCREEN_HEIGHT 768
#define FRAMEBUFFER_SCREEN_DEPTH 32

bool framebuffer_init( void );
bool framebuffer_register_rpc( void );

void framebuffer_handle_resolution( pid_t, size_t );
void framebuffer_handle_clear( pid_t, size_t );
void framebuffer_handle_render_text( pid_t, size_t );

#endif
