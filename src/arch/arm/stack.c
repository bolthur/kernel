
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

#include <stdbool.h>
#include <stdint.h>
#include <arch/arm/stack.h>
#include <core/debug/debug.h>
#include <core/panic.h>

extern void stack_supervisor_mode( void );

bool stack_is_kernel( __maybe_unused uintptr_t address ) {
  uintptr_t stack_start = ( uintptr_t )&stack_supervisor_mode;
  uintptr_t stack_end = stack_start + STACK_SIZE;
  DEBUG_OUTPUT( "start = %x, end = %x, addr = %x\r\n",
    stack_start, stack_end, address );
  return stack_start <= address && stack_end > address;
}
