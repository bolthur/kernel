
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

#include <stddef.h>
#include <stdbool.h>

#include <arch/arm/delay.h>
#include <arch/arm/mmio.h>

#include <platform/rpi/peripheral.h>
#include <platform/rpi/gpio.h>
#include <platform/rpi/mailbox/property.h>

#include <kernel/serial.h>

/**
 * @brief Initialized flag
 */
static bool serial_initialized = false;

/**
 * @brief Initialize serial port
 */
void serial_init( void ) {
  // handle initialized
  if ( serial_initialized ) {
    return;
  }

  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

  // Disable UART0.
  mmio_write( base + UARTCR, 0 );

  // Setup the GPIO pin 14 && 15
  uint32_t ra = mmio_read( base + GPFSEL1 );
  ra = ( uint32_t )( ( int32_t ) ra & ~( 7 << 12 ) );
  ra |= 4 << 12;
  ra = ( uint32_t )( ( int32_t ) ra & ~( 7 << 15 ) );
  ra |= 4 << 15;
  mmio_write( base + GPFSEL1, ra );

  // Disable pull up/down for all GPIO pins & delay for 150 cycles.
  mmio_write( base + GPPUD, 0 );
  delay( 150 );

  // Disable pull up/down for pin 14,15 & delay for 150 cycles.
  mmio_write( base + GPPUDCLK0, ( 1 << 14 ) | ( 1 << 15 ) );
  delay( 150 );

  // Write 0 to GPPUDCLK0 to make it take effect.
  mmio_write( base + GPPUDCLK0, 0 );

  // Clear pending interrupts.
  mmio_write( base + UARTICR, 0x7FF );

  // query mailbox to get uart clock rate
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_CLOCK_RATE, TAG_CLOCK_UART );
  mailbox_property_process();

  // get clock rate property
  rpi_mailbox_property_t *p = mailbox_property_get( TAG_GET_CLOCK_RATE );
  uint32_t clock_rate = p->data.buffer_u32[ 1 ];

  // calculate divider ( Divider = UART_CLOCK/(16 * Baud) )
  float divider = ( float )clock_rate / ( 16 * 115200 );
  // cast to integer for later write to ibrd
  uint32_t brd = ( uint32_t )divider;

  // calculate fractional ( (Fractional part * 64) + 0.5 )
  float fractional = divider - ( float )brd;
  // calculate fractional for later write to fbrd
  uint32_t frd = ( uint32_t )( ( fractional * 64 ) + 0.5 );

  // write baud rate
  mmio_write( base + UARTIBRD, brd );
  mmio_write( base + UARTFBRD, frd );

  // Enable FIFO & 8 bit data transmissio (1 stop bit, no parity).
  mmio_write( base + UARTLCRH, ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) );

  // Mask all interrupts.
  mmio_write(
    base + UARTIMSC,
    ( 1 << 1 ) | ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 )
    | ( 1 << 7 ) | ( 1 << 8 ) | ( 1 << 9 ) | ( 1 << 10 )
  );

  // Enable UART0, receive & transfer part of UART.
  mmio_write( base + UARTCR, ( 1 << 0 ) | ( 1 << 8 ) | ( 1 << 9 ) );

  // set flag
  serial_initialized = true;
}

/**
 * @brief Put character to serial
 *
 * @param c character to put
 */
void serial_putc( uint8_t c ) {
  // handle not initialized
  if ( ! serial_initialized ) {
    return;
  }

  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

  // Wait for UART to become ready to transmit.
  while ( 0 != ( mmio_read( base + UARTFR ) & ( 1 << 5 ) ) ) { }
  mmio_write( base + UARTDR, ( uint32_t ) c );
}

/**
 * @brief Get character from serial
 *
 * @return uint8_t Character from serial
 */
uint8_t serial_getc( void ) {
  // handle not initialized
  if ( ! serial_initialized ) {
    return 0;
  }

  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

  // Wait for UART to become ready for read
  while ( mmio_read( base + UARTFR ) & ( 1 << 4 ) ) { }

  // return data
  return ( uint8_t )mmio_read( base + UARTDR );
}

/**
 * @brief Flush serial
 */
void serial_flush( void ) {
  // handle not initialized
  if ( ! serial_initialized ) {
    return;
  }

  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

  // read from uart until as long as something is existing to flush
  while ( ! ( mmio_read( base + UARTFR ) & ( 1 << 4 ) ) ) {
    serial_getc();
  }
}
