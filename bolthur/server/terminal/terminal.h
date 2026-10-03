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

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "../../library/collection/list/list.h"
#include "../libterminal.h"
#include "../libframebuffer.h"
#include "buffer.h"

#define TERMINAL_BASE_PATH "/dev/tty"
#define TERMINAL_MAX_NUM 7

/**
 * @brief Terminal representation
 */
typedef struct terminal {
  /** terminal path */
  char path[ PATH_MAX ];
  /** circular buffer */
  circular_line_buffer_t buffer;
  /** current column */
  uint32_t col;
  /** current row */
  uint32_t row;
  /** max column */
  uint32_t max_col;
  /** max row */
  uint32_t max_row;
  /** bits per pixel */
  uint32_t bpp;
} terminal_t;

extern list_manager_t* terminal_list;
extern uint8_t* surface;
extern framebuffer_surface_allocate_t surface_data;

bool terminal_allocate_framebuffer( void );
bool terminal_init( void );
char* terminal_get_active( void );

#endif
