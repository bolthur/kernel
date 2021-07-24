/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <stdio.h>
#include <errno.h>
#include <core/syscall.h>
#include <core/panic.h>

/**
 * @fn void syscall_kernel_putc(void*)
 * @brief Kernel put character system call
 *
 * @param context
 */
void syscall_kernel_putc( void* context ) {
  printf( "%c", ( uint8_t )syscall_get_parameter( context, 0 ) );
}

/**
 * @fn void syscall_kernel_puts(void*)
 * @brief Kernel put string system call
 *
 * @param context
 */
void syscall_kernel_puts( void* context ) {
  // get parameter
  char* str = ( char* )syscall_get_parameter( context, 0 );
  int len = ( int )syscall_get_parameter( context, 1 );
  // handle errors
  if ( ! str ) {
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // print until end of string or len
  syscall_populate_success(
    context, ( size_t )printf( "%.*s", len, str )
  );
}
