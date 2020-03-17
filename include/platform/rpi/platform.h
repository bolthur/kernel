
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#if ! defined( __PLATFORM_RPI_PLATFORM__ )
#define __PLATFORM_RPI_PLATFORM__

#include <stdint.h>

#define PLATFORM_ATAG_FALLBACK_ADDR 0x00000100

enum {
  PLATFORM_USE_SOURCE_NONE = 0,
  PLATFORM_USE_SOURCE_ATAG_V1,
  PLATFORM_USE_SOURCE_ATAG_V2,
  PLATFORM_USE_SOURCE_DEVICE_TREE_V1,
  PLATFORM_USE_SOURCE_DEVICE_TREE_V2
};

typedef struct {
  uint32_t zero;
  uint32_t machine;
  uint32_t atag;
} platform_loader_parameter_t;

#endif
