
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

#include <stdio.h>
#include <stdlib.h>
#include <core/interrupt.h>

/**
 * @brief Initialize panic process
 */
void panic_init( void ) {
  interrupt_disable();
}

/**
 * @brief Panic method
 *
 * @param message Message to print
 * @param file File that invoked the panic
 * @param line Line where panic was called
 */
void __no_return panic(
  const char* restrict message,
  const char* restrict file,
  uint32_t line
) {
  // panic init
  panic_init();

  // print panic
  printf( "PANIC( %s ) at %s:%u\r\n", message, file, line );

  // abort further execution
  abort();
}
