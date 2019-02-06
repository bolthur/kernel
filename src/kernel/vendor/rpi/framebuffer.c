
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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
#include <stdint.h>
#include <stdbool.h>

#include "lib/stdc/stdlib.h"
#include "lib/stdc/string.h"
#include "lib/stdc/stdio.h"
#include "font/font.8x8.h"

#include "kernel/vendor/rpi/framebuffer.h"
#include "kernel/vendor/rpi/mailbox/property.h"

// internal variables
static bool initialized = false;
static volatile uint8_t* address;
static int32_t width = 0, height = 0, bpp = 0;
static int32_t x = 0, y = 0, pitch = 0;

/**
 * @brief Initialize framebuffer
 */
void framebuffer_init( void ) {
  rpi_mailbox_property_t* mp;

  // Initialize framebuffer
  mailbox_property_init();
  mailbox_property_add_tag( TAG_ALLOCATE_BUFFER, 16 );
  mailbox_property_add_tag( TAG_SET_PHYSICAL_SIZE, FRAMEBUFFER_SCREEN_WIDTH, FRAMEBUFFER_SCREEN_HEIGHT );
  mailbox_property_add_tag( TAG_SET_VIRTUAL_SIZE, FRAMEBUFFER_SCREEN_WIDTH, FRAMEBUFFER_SCREEN_HEIGHT );
  mailbox_property_add_tag( TAG_SET_DEPTH, FRAMEBUFFER_SCREEN_DEPTH );
  mailbox_property_add_tag( TAG_SET_PIXEL_ORDER, 0 );
  mailbox_property_add_tag( TAG_SET_VIRTUAL_OFFSET, 0, 0 );
  mailbox_property_add_tag( TAG_GET_PITCH );
  mailbox_property_add_tag( TAG_GET_PHYSICAL_SIZE );
  mailbox_property_add_tag( TAG_GET_DEPTH );
  mailbox_property_process();

  if ( ( mp = mailbox_property_get( TAG_GET_PHYSICAL_SIZE ) ) ) {
    width = mp->data.buffer_32[ 0 ];
    height = mp->data.buffer_32[ 1 ];
    printf( "Initialised Framebuffer: %dx%d ", width, height );
  }

  if ( ( mp = mailbox_property_get( TAG_GET_DEPTH ) ) ) {
    bpp = mp->data.buffer_32[ 0 ];
    printf( "%dbpp\r\n", bpp );
  }

  if ( ( mp = mailbox_property_get( TAG_GET_PITCH ) ) ) {
    pitch = mp->data.buffer_32[ 0 ];
    printf( "Pitch: %d bytes\r\n", pitch );
  }

  if ( ( mp = mailbox_property_get( TAG_ALLOCATE_BUFFER ) ) ) {
    address = ( uint8_t* )mp->data.buffer_32[ 0 ];
    printf( "Framebuffer address: 0x%08p\r\n", address );
  }

  initialized = true;
}

/**
 * @brief Internal method to put pixel
 *
 * @param x x coordinate
 * @param y y coordinate
 * @param r red value
 * @param g green value
 * @param b blue value
 * @param a alpha value
 */
static void put_pixel(
  int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a
) {
  int pixel_offset = ( x * ( bpp >> 3 ) ) + ( y * pitch );

  // Render bitmap retrieved above
  if ( bpp == 32 ) {
    // Four bytes to write
    *( uint32_t* )&address[ pixel_offset ] = ( uint32_t ) (
      ( ( r << 24 ) + ( g << 16 ) + ( b << 8 ) + a ) >> 8
    );
  } else if ( bpp == 24 ) {
    // Three bytes to write
    *( uint32_t* )&address[ pixel_offset ] = ( uint32_t ) (
      ( uint32_t )( ( r << 24 ) + ( g << 16 ) + ( b << 8 ) )
      | ( *( uint32_t* )&address[ pixel_offset ] & 0x000000FF )
    );
  } else if ( bpp == 16 ) {
    // Two bytes to write
    // Bit pack RGB565 into the 16-bit pixel offset
    *( uint16_t* )&address[ pixel_offset ] = ( uint16_t )(
      ( r >> 3 ) << 11 | ( g >> 2 ) << 5 | ( b >> 3 )
    );
  } else {
    // FIXME: Add palette mode.
  }
}

/**
 * @brief Internal method to scroll up if end has been reached
 */
static void scroll( void ) {
  uint32_t row_size = (uint32_t)( FONT_HEIGHT * pitch );
  uint32_t src = ( uint32_t )( address + row_size );
  uint32_t max_y = FRAMEBUFFER_SCREEN_HEIGHT / FONT_HEIGHT;

  // check for scrolling is really necessary
  if ( y + FONT_HEIGHT < height ) {
    // no scrolling necessary, so just increment y
    y += FONT_HEIGHT;
    return;
  }

  // move memory up
  memmove( ( void* )address, ( void* )src, max_y * row_size );

  // erase last line
  memset( ( void* )( address + ( max_y * row_size ) ), 0, row_size );

  x = 0;
}

/**
 * @brief Print character to framebuffer
 *
 * @param c character to print
 */
void framebuffer_putc( uint8_t c ) {
  // FIXME: There seems to be a bug here, when printing over line end where it shall scroll down
  // Bug is visible with a resolution of 800x600 and enabled phys memory management output

  // Don't print anything if not initialized
  if ( ! initialized ) {
    return;
  }

  // used variables
  bool set;
  uint8_t *bitmap;
  int32_t off;

  switch( c ) {
    case '\r':
      x = 0;
      break;

    case '\n':
      scroll();
      break;

    case '\t':
      off = TAB_WIDTH;
      if ( x + off > width ) {
        // determine remaining spaces
        off = x + off - width;

        // scroll if necessary
        scroll();

        // add remaining offset
        x += off;
      } else {
        x += off;
      }
      break;

    default:
      // skip not supported characters
      if ( 127 < c ) {
        return;
      }

      // check whether line break is necessary
      if ( x + FONT_WIDTH > width ) {
        scroll();
      }

      // get bitmap to render
      bitmap = font8x8_basic[ c ];

      // Render character
      for ( int32_t char_x = 0; char_x < FONT_WIDTH; char_x++ ) {
        for ( int32_t char_y = 0; char_y < FONT_HEIGHT; char_y++ ) {
          set = ( bitmap[ char_x ] & 1 << char_y );
          put_pixel(
            x + char_y,
            y + char_x,
            set ? 0xff : 0,
            set ? 0xff : 0,
            set ? 0xff : 0,
            0
          );
          continue;
        }
      }

      x += FONT_WIDTH;
  }
}
