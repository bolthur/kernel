/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <sys/bolthur.h>
#include <sys/termios.h>
#include "../../library/collection/list/list.h"

#define MAX_BUFFER_SIZE 4096

/**
 * @brief Console ring buffer
 */
typedef struct {
  /** buffer */
  char buffer[ MAX_BUFFER_SIZE ];
  /** head */
  size_t head;
  /** tail */
  size_t tail;
  /** count */
  size_t count;
} console_buffer_t;

/**
 * @brief console structure
 */
typedef struct {
  /** indicates whether console is active */
  bool active;
  /** console handler */
  pid_t handler;
  /** console path */
  char* path;
  /** in rpc number */
  size_t in;
  /** out rpc number */
  size_t out;
  /** err rpc number */
  size_t err;
  /** file descriptor */
  int fd;
  /** termios configuration */
  struct termios ios;
  /** raw buffer */
  console_buffer_t raw;
  /** cooked buffer */
  console_buffer_t cooked;
} console_t;

extern list_manager_t* console_list;

void console_destroy( console_t* );
console_t* console_get_active( void );
console_t* console_get_by_path( const char* );
void console_buffer_push( console_buffer_t*, char );
char console_buffer_pop( console_buffer_t* );

#endif
