
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if ! defined( __ARCH_ARM_V7_TASK_THREAD__ )
#define __ARCH_ARM_V7_TASK_THREAD__

#include <stdint.h>
#include <arch/arm/v7/cpu.h>

typedef struct {
  cpu_register_context_t context;
  uint32_t cpu_num;
} thread_control_block_t, *thread_control_block_ptr_t;

extern thread_control_block_t tcb_undefined;
extern thread_control_block_t tcb_software;
extern thread_control_block_t tcb_prefetch;
extern thread_control_block_t tcb_data;
extern thread_control_block_t tcb_unused;
extern thread_control_block_t tcb_irq;
extern thread_control_block_t tcb_fiq;

#endif
