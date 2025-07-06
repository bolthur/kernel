/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _ARCH_ARM_V7_REGISTER_TTBCR_H
#define _ARCH_ARM_V7_REGISTER_TTBCR_H

#include <stdint.h>

typedef union __packed {
  uint32_t raw;

  struct {
    uint32_t ttbr_split : 3;
    uint32_t sbz_0 : 1;
    union {
      uint32_t sbz_1 : 2;
      struct {
        uint32_t walk_0 : 1;
        uint32_t walk_1 : 1;
      } table;
    } disable;
    uint32_t sbz_2 : 26;
    uint32_t large_physical_address_extension : 1;
  } sd;

  struct {
    uint32_t ttbr0_size : 3;
    uint32_t sbz_0 : 4;
    uint32_t ttbr0_disable_table_walk : 1;
    uint32_t ttbr0_inner_cachability : 2;
    uint32_t ttbr0_outer_cachability : 2;
    uint32_t ttbr0_shareability : 2;
    uint32_t sbz_1 : 2;
    uint32_t ttbr1_size : 3;
    uint32_t sbz_2 : 3;
    uint32_t ttbr0_ttbr1_asid : 1;
    uint32_t ttbr1_disable_table_walk : 1;
    uint32_t ttbr1_inner_cachability : 2;
    uint32_t ttbr1_outer_cachability : 2;
    uint32_t ttbr1_shareability : 2;
    uint32_t imp : 1;
    uint32_t large_physical_address_extension : 1;
  } lpae;
} ttbcr_t;

#endif
