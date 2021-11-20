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
#include <stdbool.h>
#include <unistd.h>
#include <sys/bolthur.h>

#if ! defined( _HANDLER_H )
#define _HANDLER_H

struct console_rpc {
  uint32_t command;
  handler_t callback;
};

void handler_console_add( size_t, pid_t, size_t );
void handler_console_select( size_t, pid_t, size_t );
bool handler_register( void );

extern struct console_rpc command_list[ 3 ];

#endif
