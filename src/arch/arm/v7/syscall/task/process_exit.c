
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

#include <core/event.h>
#include <core/syscall.h>
#include <core/interrupt.h>
#include <arch/arm/v7/cpu.h>

void syscall_process_exit( void* context ) {
  // get context
  INTERRUPT_DETERMINE_CONTEXT( context )
  // transform to cpu structure
  __unused cpu_register_context_ptr_t cpu =
    ( cpu_register_context_ptr_t )context;
}
