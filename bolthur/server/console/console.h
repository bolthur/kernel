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

#include <stdbool.h>
#include <unistd.h>
#include "list.h"

#if ! defined( _CONSOLE_H )
#define _CONSOLE_H

struct console {
  bool active;
  pid_t handler;
  char* path;
  size_t in;
  size_t out;
  size_t err;
  int fd;
};
typedef struct console console_t;
typedef struct console* console_ptr_t;

extern list_manager_ptr_t console_list;

void console_destroy( console_ptr_t );
console_ptr_t console_get_active( void );

#endif
