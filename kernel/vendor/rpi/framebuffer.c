
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
#include <stdlib.h>

#include <stdio.h>

#include "vendor/rpi/framebuffer.h"
#include "vendor/rpi/mailbox/property.h"

// forward declaration of test font
extern uint8_t font8x8_basic[ 128 ][ 8 ];

// internal variables
static bool initialized = false;
static volatile uint8_t* address;
static int32_t width = 0, height = 0, bpp = 0;
static int32_t x = 0, y = 0, pitch = 0;

/**
 * @brief Initialize framebuffer
 */
void framebuffer_init( void ) {
  /*color_t current_colour;
  float cd = 0.5;
  int32_t pixel_offset;
  int32_t r, g, b, a;
  uint32_t frame_count = 0;*/
  rpi_mailbox_property_t* mp;

  /* Initialise a framebuffer... */
  mailbox_property_init();
  mailbox_property_add_tag( TAG_ALLOCATE_BUFFER );
  mailbox_property_add_tag( TAG_SET_PHYSICAL_SIZE, FRAMEBUFFER_SCREEN_WIDTH, FRAMEBUFFER_SCREEN_HEIGHT );
  mailbox_property_add_tag( TAG_SET_VIRTUAL_SIZE, FRAMEBUFFER_SCREEN_WIDTH, FRAMEBUFFER_SCREEN_HEIGHT * 2 );
  mailbox_property_add_tag( TAG_SET_DEPTH, FRAMEBUFFER_SCREEN_DEPTH );
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

  /* Never exit as there is no OS to exit to! */
  /*current_colour.r = 0;
  current_colour.g = 0;
  current_colour.b = 0;
  current_colour.a = 1.0;

  while ( 1 ) {
    current_colour.r = 0;

    // Produce a colour spread across the screen
    for ( y = 0; y < height; y++ ) {
      current_colour.r += ( float )( 1.0 / height );
      current_colour.b = 0;

      for ( x = 0; x < width; x++ ) {
        pixel_offset = ( x * ( bpp >> 3 ) ) + ( y * pitch );

        r = ( int32_t )( current_colour.r * 0xFF ) & 0xFF;
        g = ( int32_t )( current_colour.g * 0xFF ) & 0xFF;
        b = ( int32_t )( current_colour.b * 0xFF ) & 0xFF;
        a = ( int32_t )( current_colour.b * 0xFF ) & 0xFF;

        if ( bpp == 32 ) {
          // Four bytes to write
          fb[ pixel_offset++ ] = ( uint8_t )r;
          fb[ pixel_offset++ ] = ( uint8_t )g;
          fb[ pixel_offset++ ] = ( uint8_t )b;
          fb[ pixel_offset++ ] = ( uint8_t )a;
        } else if ( bpp == 24 ) {
          // Three bytes to write
          fb[ pixel_offset++ ] = ( uint8_t )r;
          fb[ pixel_offset++ ] = ( uint8_t )g;
          fb[ pixel_offset++ ] = ( uint8_t )b;
        } else if ( bpp == 16 ) {
          // Two bytes to write
          // Bit pack RGB565 into the 16-bit pixel offset
          *( uint16_t* )&fb[pixel_offset] = ( uint16_t )( ( (r >> 3) << 11 ) | ( ( g >> 2 ) << 5 ) | ( b >> 3 ) );
        } else {
          // Palette mode. TODO: Work out a colour scheme for packing rgb into an 8-bit palette!
        }

        current_colour.b += ( float )( 1.0 / width );
      }
    }

    / * Scroll through the green colour * /
    current_colour.g += cd;
    if ( current_colour.g > 1.0 ) {
      current_colour.g = 1.0;
      cd = (float)-0.5;
    } else if ( current_colour.g < 0.0 ) {
      current_colour.g = 0.0;
      cd = (float)0.5;
    }

    frame_count++;
    / *if ( calculate_frame_count ) {
      calculate_frame_count = 0;

      // Number of frames in a minute, divided by seconds per minute
      float fps = (float)frame_count / 60;
      printf( "FPS: %.2f\r\n", fps );

      frame_count = 0;
    }* /
  }*/

  initialized = true;
  printf("Test");for(;;);
}

/**
 * @brief Print character to framebuffer
 *
 * @param c character to print
 */
void framebuffer_putc( uint8_t c ) {
  // Don't print anything if not initialized
  if ( ! initialized ) {
    return;
  }

  // transform from character to int
  const char tmp[] = { c, '\0' };
  int32_t ord = atoi( tmp );

  // int32_t set, mask;

  if ( x + 8 > width ) {
    x = 0;

    // FIXME: Add handle for height limit reached
    if ( y + 8 > height ) {
      for(;;);
    } else {
      y += 8;
    }
  }

  switch( c ) {
    case '\r':
      x = 0;
      break;

    case '\n':
      x = 0;

      // FIXME: Add handle for height limit reached
      if ( y + 8 > height ) {
        for(;;);
      } else {
        y += 8;
      }
      break;

    default:
      // skip not supported characters
      if ( 127 < ord || 0 > ord ) {
        return;
      }

      // get bitmap to render
      uint8_t *bitmap = font8x8_basic[ ord ];

      for ( int32_t char_x = 0; char_x < 8; char_x++ ) {
        for ( int32_t char_y = 0; char_y < 8; char_y++ ) {
          // TODO: Render bitmap retrieved above
          ( void )bitmap;
        }
      }
  }
}
