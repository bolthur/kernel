
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

#include <stdint.h>
#include <stdio.h>

#include "kernel/panic.h"

#include "vendor/rpi/platform.h"
#include "vendor/rpi/mailbox/property.h"

/**
 * @brief Boot parameter data set during startup
 */
platform_boot_parameter_t boot_parameter_data;

/**
 * @brief Platform depending initialization routine
 */
void platform_init( void ) {
  // FIXME: Load firmware revision, board model, board revision, board serial from mailbox
  /*mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_BOARD_MODEL );
  mailbox_property_add_tag( TAG_GET_BOARD_REVISION );
  mailbox_property_add_tag( TAG_GET_FIRMWARE_VERSION );
  mailbox_property_add_tag( TAG_GET_BOARD_SERIAL );
  mailbox_property_process();*/
}

//////// REMOVE LATER STARTING FROM HERE

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480
#define SCREEN_DEPTH    16      /* 16 or 32-bit */
#define COLOUR_DELTA    (float)0.05    /* Float from 0 to 1 incremented by this amount */

typedef struct {
  float r;
  float g;
  float b;
  float a;
} colour_t;

void platform_fb_test( void ) {
  int32_t width = 0, height = 0, bpp = 0;
  int32_t x, y, pitch = 0;
  colour_t current_colour;
  volatile uint8_t* fb = NULL;
  int32_t pixel_offset;
  int32_t r, g, b, a;
  float cd = COLOUR_DELTA;
  uint32_t frame_count = 0;
  rpi_mailbox_property_t* mp;

  /* Initialise a framebuffer... */
  mailbox_property_init();
  mailbox_property_add_tag( TAG_ALLOCATE_BUFFER );
  mailbox_property_add_tag( TAG_SET_PHYSICAL_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT );
  mailbox_property_add_tag( TAG_SET_VIRTUAL_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT * 2 );
  mailbox_property_add_tag( TAG_SET_DEPTH, SCREEN_DEPTH );
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
    fb = ( uint8_t* )mp->data.buffer_32[ 0 ];
    printf( "Framebuffer address: 0x%08x\r\n", ( uintptr_t )fb );
  }

  /* Never exit as there is no OS to exit to! */
  current_colour.r = 0;
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

    /* Scroll through the green colour */
    current_colour.g += cd;
    if ( current_colour.g > 1.0 ) {
      current_colour.g = 1.0;
      cd = -COLOUR_DELTA;
    } else if ( current_colour.g < 0.0 ) {
      current_colour.g = 0.0;
      cd = COLOUR_DELTA;
    }

    frame_count++;
    /*if ( calculate_frame_count ) {
      calculate_frame_count = 0;

      // Number of frames in a minute, divided by seconds per minute
      float fps = (float)frame_count / 60;
      printf( "FPS: %.2f\r\n", fps );

      frame_count = 0;
    }*/
  }
}
