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

#if ! defined( __LIBCONSOLE__ )
#define __LIBCONSOLE__

enum console_command_type {
  CONSOLE_COMMAND_ADD = 0,
  CONSOLE_COMMAND_REMOVE,
  CONSOLE_COMMAND_SELECT,
  CONSOLE_COMMAND_CHANGE_STDIN,
  CONSOLE_COMMAND_CHANGE_STDOUT,
  CONSOLE_COMMAND_CHANGE_STDERR,
};
typedef enum console_command_type console_command_type_t;
typedef enum console_command_type* console_command_type_ptr_t;

struct console_command_add {
  char terminal[ PATH_MAX ];
  char in[ PATH_MAX ];
  char out[ PATH_MAX ];
  char err[ PATH_MAX ];
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


struct console_command {
  console_command_type_t command;
  union {
    console_command_add_t add;
    console_command_remove_t remove;
    console_command_select_t select;
    console_command_change_t change;
  };
};
typedef struct console_command console_command_t;
typedef struct console_command* console_command_ptr_t;

#endif
