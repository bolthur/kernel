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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/bolthur.h>

#if ! defined( _LIBCONSOLE_H )
#define _LIBCONSOLE_H

#define CONSOLE_ADD RPC_CUSTOM_START
#define CONSOLE_SELECT RPC_CUSTOM_START + 1

struct console_command_add {
  char terminal[ PATH_MAX ];
  size_t in;
  size_t out;
  size_t err;
  pid_t origin;
};
typedef struct console_command_add console_command_add_t;
typedef struct console_command_add* console_command_add_ptr_t;

struct console_command_remove {
  char path[ PATH_MAX ];
};
typedef struct console_command_remove console_command_remove_t;
typedef struct console_command_remove* console_command_remove_ptr_t;

struct console_command_select {
  char path[ PATH_MAX ];
};
typedef struct console_command_select console_command_select_t;
typedef struct console_command_select* console_command_select_ptr_t;

struct console_command_change {
  char terminal_path[ PATH_MAX ];
  char destination_path[ PATH_MAX ];
};
typedef struct console_command_change console_command_change_t;
typedef struct console_command_change* console_command_change_ptr_t;

#endif
