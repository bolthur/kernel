
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __KERNEL_VENDOR_RPI_PLATFORM__
#define __KERNEL_VENDOR_RPI_PLATFORM__

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
} platform_boot_parameter_t;

#endif
