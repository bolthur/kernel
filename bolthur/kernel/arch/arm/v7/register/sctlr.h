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

#include <stdint.h>

#ifndef _ARCH_ARM_V7_REGISTER_SCTLR_H
#define _ARCH_ARM_V7_REGISTER_SCTLR_H

typedef union __packed {
  uint32_t raw;
  struct {
    uint32_t mmu : 1;
    uint32_t alignment_check : 1;
    uint32_t cache_enable : 1;
    uint32_t reserved_0 : 2;
    uint32_t cp15_barrier_operations : 1;
    uint32_t reserved_1 : 1;
    uint32_t sbz_0 : 1;
    uint32_t reserved_2 : 2;
    uint32_t swp_swpb_enable : 1;
    uint32_t branch_prediction_enable : 1;
    uint32_t instruction_cache_enable : 1;
    uint32_t vectors : 1;
    uint32_t round_robin_select : 1;
    uint32_t reserved_3 : 2;
    uint32_t hardware_access_flag : 1;
    uint32_t reserved_4 : 1;
    uint32_t wxn : 1;
    uint32_t uwxn : 1;
    uint32_t fast_interrupt_configuration_enable : 1;
    uint32_t reserved_5 : 2;
    uint32_t interrupt_vector_enable : 1;
    uint32_t exception_endianess : 1;
    uint32_t reserved_6 : 1;
    uint32_t non_maskable_fiq_support : 1;
    uint32_t tex_remap_enable : 1;
    uint32_t access_flag_enable : 1;
    uint32_t thumb_exception_enable : 1;
    uint32_t resered_7 : 1;
  } data;
} sctlr_t;

#endif
