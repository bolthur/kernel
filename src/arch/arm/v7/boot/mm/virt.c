
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

#include <stdint.h>

#include <arch/arm/mm/virt.h>
#include <core/entry.h>
#include <arch/arm/v7/boot/mm/virt/long.h>
#include <arch/arm/v7/boot/mm/virt/short.h>

#include <platform/rpi/gpio.h>

/**
 * @brief Supported mode
 */
static uint32_t supported_mode __bootstrap_data;

void __bootstrap boot_io_out32( uint32_t port, uint32_t val ) {
  __asm__( "dmb" ::: "memory" );
  *( volatile uint32_t* )( port ) = val;
  __asm__( "dmb" ::: "memory" );
}

uint32_t __bootstrap boot_io_in32( uint32_t port ) {
  __asm__( "dmb" ::: "memory" );
  return *( volatile uint32_t* )( port );
}

void __bootstrap boot_io_out8( uint32_t port, uint8_t val ) {
  boot_io_out32( port, ( uint32_t )val );
}

void __bootstrap boot_putc( uint8_t c ) {
  // get peripheral base
  uint32_t base = 0x3F000000;

  // Wait for UART to become ready to transmit.
  while ( 0 != ( boot_io_in32( base + UARTFR ) & ( 1 << 5 ) ) ) { }
  boot_io_out8( base + UARTDR, c );
}

/**
 * @brief Method wraps setup of short / long descriptor mode
 */
void __bootstrap boot_virt_setup( void ) {
  // get paging support from mmfr0
  __asm__ __volatile__(
    "mrc p15, 0, %0, c0, c1, 4"
    : "=r" ( supported_mode )
    : : "cc"
  );

  boot_putc( 'a' );

  // strip out everything not needed
  supported_mode &= 0xF;

  // check for invalid paging support
  if (
    ! (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_PXN == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode
    )
  ) {
    boot_putc( 'b' );
    return;
  }

  // get memory size from mmfr3
  uint32_t reg;
  __asm__ __volatile__(
    "mrc p15, 0, %0, c0, c1, 7"
    : "=r" ( reg )
    : : "cc"
  );
  boot_putc( 'c' );

  // get only cpu address bus size
  reg = ( reg >> 24 ) & 0xf;
  // use short if physical address bus is not 36 bit at least
  if ( 0 == reg ) {
    supported_mode = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
  }

  boot_putc( 'd' );
  // kick start
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode ) {
    boot_virt_setup_long();
  } else {
    boot_virt_setup_short();
  }

  boot_putc( 'e' );
  // setup platform related
  boot_virt_platform_setup();

  boot_putc( 'f' );

  // enable initial mapping
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode ) {
    boot_virt_enable_long();
  } else {
    boot_virt_enable_short();
  }

  boot_putc( 'g' );
}

/**
 * @brief Mapper function using short or long descriptor mapping depending on support
 *
 * @param phys physical address
 * @param virt virtual address
 */
void __bootstrap boot_virt_map( uint64_t phys, uintptr_t virt ) {
  // check for invalid paging support
  if (
    ! (
      ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_PXN == supported_mode
      || ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode
    )
  ) {
    return;
  }

  // map it
  if ( ID_MMFR0_VSMA_V7_PAGING_LPAE == supported_mode ) {
    boot_virt_map_long( phys, virt );
  } else {
    boot_virt_map_short( ( uintptr_t )phys, virt );
  }
}
