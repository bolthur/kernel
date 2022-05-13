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

#include <errno.h>
#include "../lib/stdio.h"
#include "../lib/stdlib.h"
#include "../lib/string.h"
#include "../syscall.h"
#if defined( PRINT_SYSCALL )
  #include "../debug/debug.h"
#endif

/**
 * @fn void syscall_kernel_putc(void*)
 * @brief Kernel put character system call
 *
 * @param context
 */
void syscall_kernel_putc( void* context ) {
  int written = printf( "%c", ( uint8_t )syscall_get_parameter( context, 0 ) );
  syscall_populate_success( context, ( size_t )written );
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
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "str = %#p, len = %zu\r\n", str, len )
  #endif
  // handle invalid string ( NULL )
  if ( ! str || ! syscall_validate_address( ( uintptr_t )str, len ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "validation of parameters failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Allocate memory for unsafe copy!\r\n" )
  #endif
  // allocate space for duplicate and check for error
  char* data_dup = ( char* )malloc( sizeof( char ) * ( len + 1 ) );
  if ( ! data_dup ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Allocation failed!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMEM );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Clearing allocated memory!\r\n" )
  #endif
  memset( data_dup, 0, sizeof( char ) * ( len + 1 ) );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Unsafe copy!\r\n" )
  #endif
  // copy over
  if ( ! memcpy_unsafe_src( data_dup, str, len ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Unsafe copy failed!\r\n" )
    #endif
    free( data_dup );
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
  // print somewhere
  int written = printf( "%.*s", len, data_dup );
  // free data_dup and return written amount
  free( data_dup );
  // print until end of string or len
  syscall_populate_success( context, ( size_t )written );
}
