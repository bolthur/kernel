/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include <sys/bolthur.h>
#include <sys/mman.h>
#include "mmio.h"
#include "barrier.h"
#include "constants.h"

// initial setup of peripheral base
#if defined( BCM2709 ) || defined( BCM2710 )
  #define PERIPHERAL_BASE 0x3F000000
  #define PERIPHERAL_SIZE 0xFFFFFF
#else
  #define PERIPHERAL_BASE 0x20000000
  #define PERIPHERAL_SIZE 0xFFFFFF
#endif

static void* mmio_start = nullptr;

bool mmio_init( void ) {
  EARLY_STARTUP_PRINT( "PERIPHERAL_BASE = %#x\r\n", PERIPHERAL_BASE )
  // map whole mmio area
  void* tmp = mmap(
    ( void* )( PERIPHERAL_BASE + PERIPHERAL_USB_OFFSET ),
    0x1000,
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
  // return success
  return true;
}

/**
 * @fn uint32_t mmio_read(uintptr_t)
 * @brief Perform single mmio read
 *
 * @param address
 * @return
 */
uint32_t mmio_read(const uintptr_t address ) {
  // determine read begin and end address since address contains only an offset
  auto volatile const read_begin = ( void* )( ( uintptr_t )mmio_start + address );
  // barrier
  barrier_dmb();
  // read word
  return *( volatile uint32_t* )read_begin;
}

/**
 * @fn void mmio_write(uintptr_t, uint32_t)
 * @brief Perform single mmio write
 *
 * @param address
 * @param data
 */
void mmio_write(const uintptr_t address, const uint32_t data ) {
  // determine write begin and end address since address contains only an offset
  auto volatile const write_begin = ( void* )( ( uintptr_t )mmio_start + address );
  // barrier, write and barrier
  barrier_dmb();
  *( volatile uint32_t* )write_begin  = data;
  barrier_dmb();
}
