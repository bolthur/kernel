
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

#include <stdint.h>

#include "lib/stdc/stdio.h"
#include "kernel/kernel/irq.h"

/**
 * @brief Panic method
 *
 * @param message Message to print
 * @param file File that invoked the panic
 * @param line Line where panic was called
 */
void panic( const char* restrict message, const char* restrict file, uint32_t line ) {
  // disable interrupts
  irq_disable();

  // print panic
  printf( "PANIC( %s ) at %s:%d\r\n", message, file, line );

  // loop endless
  while ( 1 ) {}
}

/**
 * @brief Panic assertion
 *
 * @param file File that invoked the panic assert
 * @param line Line where function was called
 * @param desc Additional description
 */
void panic_assert( const char* restrict file, uint32_t line, const char* restrict desc ) {
  // disable interrupts
  irq_disable();

  // print assertion failed
  printf( "ASSERTION-FAILED( %s ) at %s:%d\r\n", desc, file, line );

  // loop endless
  while ( 1 ) {}
}
