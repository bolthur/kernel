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

#ifndef _ARCH_ARM_V7_REGISTER_DACR_H
#define _ARCH_ARM_V7_REGISTER_DACR_H

#include <stdint.h>

typedef union __packed {
  uint32_t raw;
  struct {
    uint32_t d0 : 2;
    uint32_t d1 : 2;
    uint32_t d2 : 2;
    uint32_t d3 : 2;
    uint32_t d4 : 2;
    uint32_t d5 : 2;
    uint32_t d6 : 2;
    uint32_t d7 : 2;
    uint32_t d8 : 2;
    uint32_t d9 : 2;
    uint32_t d10 : 2;
    uint32_t d11 : 2;
    uint32_t d12 : 2;
    uint32_t d13 : 2;
    uint32_t d14 : 2;
    uint32_t d15 : 2;
  } data;
} dacr_t;

#endif
