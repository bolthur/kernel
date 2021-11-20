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

#include "collection/list.h"

#if ! defined( _TERMINAL_H )
#define _TERMINAL_H

#define TERMINAL_BASE_PATH "/dev/tty"
#define TERMINAL_MAX_PATH 32
#define TERMINAL_MAX_NUM 7

struct terminal {
  char path[ TERMINAL_MAX_PATH ];
  uint16_t* buffer;
  uint32_t col;
  uint32_t row;
  uint32_t max_col;
  uint32_t max_row;
  uint32_t bpp;
};
typedef struct terminal terminal_t;
typedef struct terminal* terminal_ptr_t;

extern list_manager_ptr_t terminal_list;

bool terminal_init( void );
void terminal_scroll( terminal_ptr_t );
uint32_t terminal_push( terminal_ptr_t, const char*, uint32_t*, uint32_t*, uint32_t*, uint32_t* );

#endif
