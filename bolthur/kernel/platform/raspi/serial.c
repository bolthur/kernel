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

#include <stdbool.h>
#include <limits.h>

#include <arch/arm/delay.h>

#include <platform/raspi/peripheral.h>
#include <platform/raspi/gpio.h>
#include <platform/raspi/mailbox/property.h>

#include <io.h>
#include <serial.h>
#include <interrupt.h>
#include <event.h>
#if defined( PRINT_SERIAL )
  #include <debug/debug.h>
#endif

#define MAX_SERIAL_BUFFER 500

/**
 * @brief Initialized flag
 */
static bool serial_initialized = false;

/**
 * @brief output buffer used for formatting via sprintf
 */
static uint8_t serial_buffer[ MAX_SERIAL_BUFFER ];

/**
 * @brief Current buffer index
 */
static uint32_t index;

/**
 * @brief Method to get serial buffer
 *
 * @return uint8_t*
 */
uint8_t* serial_get_buffer( void ) {
  return serial_buffer;
}

/**
 * @brief Flush serial buffer
 */
void serial_flush_buffer( void ) {
  index = 0;
  serial_buffer[ index ] = '\0';
}

/**
 * @brief Initialize serial port
 */
void serial_init( void ) {
  // handle initialized
  if ( serial_initialized ) {
    return;
  }

  // reset buffer index
  index = 0;

  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );

  // Disable UART0.
  io_out32( base + UARTCR, 0 );

  // Set up the GPIO pin 14 && 15
  uint32_t ra = io_in32( base + GPFSEL1 );
  ra = ( uint32_t )( ( int32_t ) ra & ~( 7 << 12 ) );
  ra |= 4 << 12;
  ra = ( uint32_t )( ( int32_t ) ra & ~( 7 << 15 ) );
  ra |= 4 << 15;
  io_out32( base + GPFSEL1, ra );

  // Disable pull up/down for all GPIO pins & delay for 150 cycles.
  io_out32( base + GPPUD, 0 );
  delay( 150 );

  // Disable pull up/down for pin 14,15 & delay for 150 cycles.
  io_out32( base + GPPUDCLK0, ( 1 << 14 ) | ( 1 << 15 ) );
  delay( 150 );

  // Write 0 to GPPUDCLK0 to make it take effect.
  io_out32( base + GPPUDCLK0, 0 );

  // Clear pending interrupts.
  io_out32( base + UARTICR, 0x7FF );

  // query mailbox to get uart clock rate
  mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_CLOCK_RATE, TAG_CLOCK_UART );
  mailbox_property_process();

  // get clock rate property
  raspi_mailbox_property_t* p = mailbox_property_get( TAG_GET_CLOCK_RATE );
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
  io_out32( base + UARTIBRD, brd );
  io_out32( base + UARTFBRD, frd );

  // Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
  io_out32( base + UARTLCRH, ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) );

  // Mask incoming interrupt only
  #if defined( REMOTE_DEBUG )
    io_out32( base + UARTIMSC, ( 1 << 4 ) );
  #endif
  // Enable UART0, receive & transfer part of UART.
  io_out32( base + UARTCR, ( 1 << 0 ) | ( 1 << 8 ) | ( 1 << 9 ) );

  // set flag
  serial_initialized = true;
}

/**
 * @brief serial clear callback
 *
 * @param context cpu context
 */
static void serial_clear( __unused void* context ) {
  // debug output
  #if defined( PRINT_SERIAL )
    DEBUG_OUTPUT( "Clear interrupt source for serial!\r\n" )
  #endif
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // get interrupt state
  uint32_t state = io_in32( base + UARTMIS );

  // loop until flag will be reset
  while (
    (
      ( io_in32( base + UARTRIS ) & ( 1 << 4 ) )
      && ! ( io_in32( base + UARTFR ) & ( 1 << 4 ) )
    )
  ) {
    // clear out serial buffer to prevent overrun
    if ( MAX_SERIAL_BUFFER <= ( index + 1 ) ) {
      serial_flush_buffer();
    }

    // read data register
    uint16_t data_register = io_in16( base + UARTDR );
    // extract error related stuff
    uint8_t error = ( uint8_t )( data_register >> CHAR_BIT );
    // extract character
    uint8_t character = ( uint8_t )data_register;

    // handle error by dropping packet
    if ( 0 != error ) {
      // flush buffer
      serial_flush_buffer();
      // stop it
      break;
    }

    // read character
    serial_buffer[ index++ ] = character;
    // add ending character
    serial_buffer[ index ] = '\0';
  }

  // clear pending interrupts.
  io_out32( base + UARTICR, state );
  // trigger serial event
  event_enqueue( EVENT_SERIAL, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @brief register serial interrupt
 */
bool serial_register_interrupt( void ) {
  // get peripheral base
  uint32_t base = ( uint32_t )peripheral_base_get( PERIPHERAL_GPIO );
  // register interrupt
  if ( ! interrupt_register_handler( 57, serial_clear, NULL, INTERRUPT_FAST, true, false ) ) {
    return false;
  }
  // mask interrupt
  io_out32( base + INTERRUPT_FIQ_CONTROL, 57 | 0x80 );
  // flush it
  serial_flush();
  return true;
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
  while ( 0 != ( io_in32( base + UARTFR ) & ( 1 << 5 ) ) ) { }
  io_out8( base + UARTDR, c );
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
  while ( io_in32( base + UARTFR ) & ( 1 << 4 ) ) { }

  // return data
  return io_in8( base + UARTDR );
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
  while ( ! ( io_in32( base + UARTFR ) & ( 1 << 4 ) ) ) {
    serial_getc();
  }
}
