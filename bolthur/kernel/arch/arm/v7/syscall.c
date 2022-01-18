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

#include "../../../lib/assert.h"
#include "../../../syscall.h"
#include "../../../interrupt.h"
#include "cpu.h"

/**
 * @fn void syscall_populate_success(void*, size_t)
 * @brief Populate single return to caller
 *
 * @param context
 * @param value
 */
void syscall_populate_success( void* context, size_t value ) {
  // get context
  INTERRUPT_DETERMINE_CONTEXT( context )
  // get cpu context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context ;
  // set return values
  cpu->reg.r0 = value;
  cpu->reg.r1 = 0;
}

/**
 * @fn void syscall_populate_error(void*, size_t)
 * @brief Populate error return to caller
 *
 * @param context
 * @param error
 */
void syscall_populate_error( void* context, size_t error ) {
  // get context
  INTERRUPT_DETERMINE_CONTEXT( context )
  // get cpu context
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context ;
  // set return values
  cpu->reg.r0 = 0;
  cpu->reg.r1 = error;
}

/**
 * @fn size_t syscall_get_parameter(void*, int32_t)
 * @brief Helper to get parameter of context
 *
 * @param context
 * @param num
 * @return
 */
size_t syscall_get_parameter( void* context, int32_t num ) {
  // get context
  INTERRUPT_DETERMINE_CONTEXT( context )
  // transform to cpu structure
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
  // number sanitize
  assert( num >= R0 && num <= CPSR )
  // return value
  return ( size_t )cpu->raw[ num ];
}
