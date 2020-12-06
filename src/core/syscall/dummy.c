
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

#include <stdio.h>
#include <core/syscall.h>
#include <core/panic.h>

/**
 * @brief Dummy system call for testing purposes
 *
 * @param context
 *
 * @todo remove with final syscall implementation
 */
void syscall_dummy_putc( void* context ) {
  printf( "%c", ( uint8_t )syscall_get_parameter( context, 0 ) );
}

/**
 * @brief Dummy system call for testing purposes
 *
 * @param context
 *
 * @todo remove with final syscall implementation
 */
void syscall_dummy_puts( void* context ) {
  // get parameter
  // int file = ( int )syscall_get_parameter( context, 0 );
  char* str = ( char* )syscall_get_parameter( context, 1 );
  int len = ( int )syscall_get_parameter( context, 2 );
  // handle errors
  if ( ! str ) {
    syscall_populate_single_return( context, ( uintptr_t )-1 );
    return;
  }
  // print until end of string or len
  syscall_populate_single_return(
    context, ( uintptr_t )printf( "%.*s", len, str )
  );
}
