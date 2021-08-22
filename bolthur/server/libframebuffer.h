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

#if ! defined( __LIBFRAMEBUFFER__ )
#define __LIBFRAMEBUFFER__

enum framebuffer_command_type {
  FRAMEBUFFER_GET_RESOLUTION = 0,
  FRAMEBUFFER_CLEAR,
  FRAMEBUFFER_RENDER_TEXT,
};
typedef enum framebuffer_command_type framebuffer_command_type_t;
typedef enum framebuffer_command_type* framebuffer_command_type_ptr_t;

struct framebuffer_render_text {
  uint32_t start_x;
  uint32_t start_y;
  uint32_t font_width;
  uint32_t font_height;
  char text[ MAX_WRITE_LEN ];
};
typedef struct framebuffer_render_text framebuffer_render_text_t;
typedef struct framebuffer_render_text* framebuffer_render_text_ptr_t;

struct framebuffer_command {
  framebuffer_command_type_t command;
  union {
    framebuffer_render_text_t text;
  };
};
typedef struct framebuffer_command framebuffer_command_t;
typedef struct framebuffer_command* framebuffer_command_ptr_t;

struct framebuffer_resolution {
  int32_t success;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
};
typedef struct framebuffer_resolution framebuffer_resolution_t;
typedef struct framebuffer_resolution* framebuffer_resolution_ptr_t;

#endif
