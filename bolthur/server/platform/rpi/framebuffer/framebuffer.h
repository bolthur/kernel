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

#include <sys/bolthur.h>

#if ! defined( _FRAMEBUFFER_H )
#define _FRAMEBUFFER_H

#define FRAMEBUFFER_SCREEN_WIDTH 1024
#define FRAMEBUFFER_SCREEN_HEIGHT 768
#define FRAMEBUFFER_SCREEN_DEPTH 32
#define BYTE_PER_PIXEL ( FRAMEBUFFER_SCREEN_DEPTH / CHAR_BIT )

struct framebuffer_rpc {
  uint32_t command;
  rpc_handler_t callback;
};

bool framebuffer_init( void );
bool framebuffer_register_rpc( void );
void framebuffer_flip( void );

void framebuffer_handle_resolution( size_t, pid_t, size_t, size_t );
void framebuffer_handle_clear( size_t, pid_t, size_t, size_t );
void framebuffer_handle_render_surface( size_t, pid_t, size_t, size_t );
void framebuffer_handle_flip( size_t, pid_t, size_t, size_t );

extern struct framebuffer_rpc command_list[ 4 ];

#endif
