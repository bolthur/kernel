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

#if ! defined( _LIBFRAMEBUFFER_H )
#define _LIBFRAMEBUFFER_H

#define FRAMEBUFFER_GET_RESOLUTION RPC_CUSTOM_START
#define FRAMEBUFFER_CLEAR FRAMEBUFFER_GET_RESOLUTION + 1
#define FRAMEBUFFER_RENDER_SURFACE FRAMEBUFFER_CLEAR + 1
#define FRAMEBUFFER_FLIP FRAMEBUFFER_RENDER_SURFACE+ 1

struct framebuffer_render_surface {
  uint32_t scroll_y;
  uint32_t x;
  uint32_t y;
  uint32_t max_x;
  uint32_t max_y;
  uint32_t bpp;
  uint8_t data[];
};
typedef struct framebuffer_render_surface framebuffer_render_surface_t;
typedef struct framebuffer_render_surface* framebuffer_render_surface_ptr_t;

struct framebuffer_resolution {
  int32_t success;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
};
typedef struct framebuffer_resolution framebuffer_resolution_t;
typedef struct framebuffer_resolution* framebuffer_resolution_ptr_t;

#endif
