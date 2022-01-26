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

#include <limits.h>
#include <errno.h>
#include <sys/mman.h>
#include "../libperipheral.h"
#include "mmio.h"

void* mmio_start = NULL;
void* mmio_end = NULL;

/**
 * @fn void dmb(void)
 * @brief Data memory barrier
 */
static void dmb( void ) {
  #if defined( BCM2835 )
    __asm__ __volatile__ ( "dmb" ::: "memory" );
  #else
    __asm__ __volatile__ (
      "mcr p15, #0, %[zero], c7, c10, #5"
      : : [ zero ] "r" ( 0 )
      : "memory"
    );
  #endif
}

/**
 * @fn bool mmio_setup(void)
 * @brief Prepare and setup mmio
 *
 * @return
 */
bool mmio_setup( void ) {
  // map whole mmio area
  void* tmp = mmap(
    ( void* )PERIPHERAL_BASE,
    PERIPHERAL_SIZE,
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PHYSICAL | MAP_DEVICE,
    -1,
    0
  );
  // handle possible error
  if( MAP_FAILED == tmp ) {
    return false;
  }
  // set mmio start address
  mmio_start = tmp;
  mmio_end = ( void* )( ( uintptr_t )tmp + PERIPHERAL_SIZE );
  // return success
  return true;
}

/**
 * @fn void mmio_read*(uintptr_t, size_t)
 * @brief Perform mmio read
 *
 * @param address
 * @param len
 *
 * @exception EINVAL invalid address has been passed
 * @exception EINVAL invalid size has been passed
 */
void* mmio_read( uintptr_t address, size_t len ) {
  // determine read begin and end address since address contains only an offset
  void* read_begin = ( void* )( ( uintptr_t )mmio_start + address );
  void* read_end = ( void* )( ( uintptr_t )mmio_start + address + len );
  // check for in range
  if ( read_end > mmio_end || read_begin > mmio_end ) {
    errno = EINVAL;
    return NULL;
  }
  // check for multiple of word
  if ( len % sizeof( uint32_t ) ) {
    errno = EINVAL;
    return NULL;
  }
  // allocate space for read return
  uint32_t* ret = malloc( len );
  // handle error
  if ( ! ret ) {
    errno = ENOMEM;
    return NULL;
  }
  // determine number of words to read
  size_t num_word = len / sizeof( uint32_t );
  uintptr_t offset = 0;
  // read word per word
  for ( size_t i = 0; i < num_word; i++, offset += sizeof( uint32_t ) ) {
    // barrier
    dmb();
    // read word
    ret[ i ] = *( volatile uint32_t* )(
      ( uintptr_t )read_begin + offset
    );
  }
  // return copied content
  return ret;
}

/**
 * @fn bool mmio_write(uintptr_t, uint32_t*, size_t)
 * @brief Perform mmio write
 *
 * @param address
 * @param data
 * @param len
 * @return
 *
 * @exception EINVAL invalid address has been passed
 */
bool mmio_write( uintptr_t address, uint32_t* data, size_t len ) {
  // determine write begin and end address since address contains only an offset
  void* write_begin = ( void* )( ( uintptr_t )mmio_start + address );
  void* write_end = ( void* )( ( uintptr_t )mmio_start + address + len );
  // check for in range
  if ( write_end > mmio_end || write_begin > mmio_end ) {
    errno = EINVAL;
    return false;
  }
  // check for multiple of word
  if ( len % sizeof( uint32_t ) ) {
    errno = EINVAL;
    return false;
  }
  // determine number of words to read
  size_t num_word = len / sizeof( uint32_t );
  uintptr_t offset = 0;
  // read word per word
  for ( size_t i = 0; i < num_word; i++, offset += sizeof( uint32_t ) ) {
    // barrier, write and barrier
    dmb();
    *( volatile uint32_t* )(
      ( uintptr_t )write_begin + offset
    ) = data[ i ];
    dmb();
  }
  // return copied content
  return true;
}
