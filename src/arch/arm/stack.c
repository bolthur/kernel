
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

#include <stdbool.h>
#include <stdint.h>
#include <arch/arm/stack.h>
#include <core/debug/debug.h>
#include <core/panic.h>
#include <core/stack.h>

extern void stack_supervisor_mode( void );

/**
 * @brief Determines whether stack is in kernel or user
 *
 * @param uintptr_t
 * @return true
 * @return false
 */
bool stack_is_kernel( uintptr_t address ) {
  uintptr_t stack_start = ( uintptr_t )&stack_supervisor_mode;
  uintptr_t stack_end = stack_start + STACK_SIZE;
  return stack_start <= address && stack_end > address;
}
