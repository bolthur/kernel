/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#if ! defined( _ARCH_ARM_V7_INTERRUPT_VECTOR_H )
#define _ARCH_ARM_V7_INTERRUPT_VECTOR_H

#include <assert.h>
#include <stack.h>
#include <arch/arm/v7/cpu.h>

void interrupt_vector_table( void );
void interrupt_ensure_kernel_stack( void );

void vector_data_abort_handler( cpu_register_context_ptr_t );
void vector_fast_interrupt_handler( cpu_register_context_ptr_t );
void vector_interrupt_handler( cpu_register_context_ptr_t );
void vector_prefetch_abort_handler( cpu_register_context_ptr_t );
void vector_svc_handler( cpu_register_context_ptr_t );
void vector_undefined_instruction_handler( cpu_register_context_ptr_t );

#endif
