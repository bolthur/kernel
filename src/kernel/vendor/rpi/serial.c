
/**
 * mist-system/kernel
 * Copyright (C) 2017 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// default includes
#include <stddef.h>

// specific architecture related
#if defined( ARCH_ARM_V7 )
#endif

// common architecture related
#if defined( ARCH_ARM )
  #include <arch/arm/delay.h>
  #include <arch/arm/mmio.h>
#endif

// specific platform related
#include <vendor/rpi/gpio.h>

// common vendor related

// normal
#include <serial.h>

void serial_init( void ) {
  // Disable UART0.
  mmio_write( UARTCR, 0 );

  // Setup the GPIO pin 14 && 15
  uint32_t ra = mmio_read( GPFSEL1 );
  ra &= ~( 7 << 12 );
  ra |= 4 << 12;
  ra &= ~( 7 << 15 );
  ra |= 4 << 15;
  mmio_write( GPFSEL1, ra );

  // Disable pull up/down for all GPIO pins & delay for 150 cycles.
  mmio_write( GPPUD, 0 );
  delay( 150 );

  // Disable pull up/down for pin 14,15 & delay for 150 cycles.
  mmio_write( GPPUDCLK0, ( 1 << 14 ) | ( 1 << 15 ) );
  delay( 150 );

  // Write 0 to GPPUDCLK0 to make it take effect.
  mmio_write( GPPUDCLK0, 0 );

  // Clear pending interrupts.
  mmio_write( UARTICR, 0x7FF );

  // Set integer & fractional part of baud rate.
  // Divider = UART_CLOCK/(16 * Baud)
  // Fraction part register = (Fractional part * 64) + 0.5
  // UART_CLOCK = 3000000; Baud = 115200.

  // Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
  mmio_write( UARTIBRD, 1 );
  // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
  mmio_write( UARTFBRD, 40 );

  // Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
  mmio_write( UARTLCRH, ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) );

  // Mask all interrupts.
  mmio_write( UARTIMSC, ( 1 << 1 ) | ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) | ( 1 << 7 ) | ( 1 << 8 ) | ( 1 << 9 ) | ( 1 << 10 ) );

  // Enable UART0, receive & transfer part of UART.
  mmio_write( UARTCR, ( 1 << 0 ) | ( 1 << 8 ) | ( 1 << 9 ) );
}

void serial_putc( uint8_t c ) {
  // Wait for UART to become ready to transmit.
  while ( 0 != ( mmio_read( UARTFR ) & ( 1 << 5 ) ) ) { }
  mmio_write( UARTDR, ( uint32_t ) c );
}

uint8_t serial_getc( void ) {
  // Wait for UART to become ready for read
  while ( mmio_read( UARTFR ) & ( 1 << 4 ) ) { }

  // return data
  return ( uint8_t )mmio_read( UARTDR );
}

void serial_flush( void ) {
  // read from uart until as long as something is existing to flush
  while ( ! ( mmio_read( UARTFR ) & ( 1 << 4 ) ) ) {
    serial_getc();
  }
}
