
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>
#include <core/debug/debug.h>
#include <platform/rpi/framebuffer.h>
#include <platform/rpi/mailbox/property.h>

#include <font/font.h>

#include <core/mm/phys.h>
#include <arch/arm/barrier.h>
#include <core/mm/virt.h>

/**
 * @brief Initialized flag
 */
static bool framebuffer_initialized = false;

/**
 * @brief Framebuffer address
 */
static volatile uint8_t* framebuffer_address = NULL;

/**
 * @brief Framebuffer size
 */
static uint32_t framebuffer_size = 0;

/**
 * @brief Resolution width
 */
static int32_t resolution_width = 0;

/**
 * @brief Resolution height
 */
static int32_t resolution_height = 0;

/**
 * @brief Bits per pixel
 */
static int32_t framebuffer_bpp = 0;

/**
 * @brief X coordinate
 */
static int32_t coordinate_x = 0;

/**
 * @brief Y coordinate
 */
static int32_t coordinate_y = 0;

/**
 * @brief pitch
 */
static int32_t pitch = 0;

/**
 * @brief Initialize framebuffer
 */
void framebuffer_init( void ) {
  rpi_mailbox_property_t* mp;

  // Initialize framebuffer
  mailbox_property_init();
  mailbox_property_add_tag( TAG_SET_PHYSICAL_SIZE, FRAMEBUFFER_SCREEN_WIDTH, FRAMEBUFFER_SCREEN_HEIGHT );
  mailbox_property_add_tag( TAG_SET_VIRTUAL_SIZE, FRAMEBUFFER_SCREEN_WIDTH, FRAMEBUFFER_SCREEN_HEIGHT );
  mailbox_property_add_tag( TAG_SET_DEPTH, FRAMEBUFFER_SCREEN_DEPTH );
  mailbox_property_add_tag( TAG_SET_PIXEL_ORDER, 0 );
  mailbox_property_add_tag( TAG_ALLOCATE_BUFFER, 16 );
  mailbox_property_add_tag( TAG_SET_VIRTUAL_OFFSET, 0, 0 );
  mailbox_property_add_tag( TAG_GET_PITCH );
  mailbox_property_add_tag( TAG_GET_PHYSICAL_SIZE );
  mailbox_property_add_tag( TAG_GET_DEPTH );
  mailbox_property_process();

  if ( ( mp = mailbox_property_get( TAG_GET_PHYSICAL_SIZE ) ) ) {
    resolution_width = mp->data.buffer_32[ 0 ];
    resolution_height = mp->data.buffer_32[ 1 ];
  }

  if ( ( mp = mailbox_property_get( TAG_GET_DEPTH ) ) ) {
    framebuffer_bpp = mp->data.buffer_32[ 0 ];
  }

  if ( ( mp = mailbox_property_get( TAG_GET_PITCH ) ) ) {
    pitch = mp->data.buffer_32[ 0 ];
  }

  if ( ( mp = mailbox_property_get( TAG_ALLOCATE_BUFFER ) ) ) {
    framebuffer_base_set( mp->data.buffer_u32[ 0 ] );
    framebuffer_size = mp->data.buffer_u32[ 1 ];
  }

  // map initially
  uintptr_t start = framebuffer_base_get();
  uintptr_t end = framebuffer_end_get();
  while ( start < end ) {
    boot_virt_map( ( uint64_t )start, start );
    start += PAGE_SIZE;
  }

  // set flag
  framebuffer_initialized = true;
}

/**
 * @brief Method to get framebuffer base address
 *
 * @return uintptr_t
 */
uintptr_t framebuffer_base_get( void ) {
  return ( uintptr_t )framebuffer_address;
}

/**
 * @brief Method to set framebuffer base address
 *
 * @param addr
 */
void framebuffer_base_set( uintptr_t addr ) {
  framebuffer_address = ( volatile uint8_t* )addr;
}

/**
 * @brief Get end address of frame buffer
 *
 * @return void*
 */
uintptr_t framebuffer_end_get( void ) {
  return ( uintptr_t )( framebuffer_address + framebuffer_size );
}

/**
 * @brief Internal method to put pixel
 *
 * @param x
 * @param y
 * @param r
 * @param g
 * @param b
 */
static void put_pixel(
  int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b
) {
  // calculate offset
  int32_t pixel_offset = ( x * ( framebuffer_bpp >> 3 ) ) + ( y * pitch );
  // Bit pack RGB565 into the 16-bit pixel offset
  *( uint16_t* )&framebuffer_address[ pixel_offset ] = ( uint16_t )(
    ( r >> 3 ) << 11 | ( g >> 2 ) << 5 | ( b >> 3 )
  );
}

/**
 * @brief Internal method to scroll up if end has been reached
 *
 * @todo optimize scrolling
 */
static void scroll( void ) {
  // calculate row size
  uint32_t row_size = (uint32_t)( FONT_HEIGHT * pitch );
  // src line
  uint32_t src = ( uint32_t )( framebuffer_address + row_size );
  // max y row
  uint32_t max_y = FRAMEBUFFER_SCREEN_HEIGHT / FONT_HEIGHT - 1;

  // check for scrolling is really necessary
  if ( coordinate_y + FONT_HEIGHT < resolution_height ) {
    // increment y coordinate
    coordinate_y += FONT_HEIGHT;
    // reset x coordinate
    coordinate_x = 0;
    // skip rest
    return;
  }

  // move memory up
  memmove( ( void* )framebuffer_address, ( void* )src, max_y * row_size );
  // erase last line
  memset( ( void*  )( framebuffer_address + ( max_y * row_size ) ), 0, row_size );
  // reset x
  coordinate_x = 0;
}

/**
 * @brief Print character to framebuffer
 *
 * @param c character to print
 */
void framebuffer_putc( uint8_t c ) {
  // ensure initialized environment
  if ( ! framebuffer_initialized ) {
    return;
  }

  // used variables
  int32_t offset;

  switch( c ) {
    case '\r':
      coordinate_x = 0;
      break;

    case '\n':
      scroll();
      break;

    case '\t':
      offset = TAB_WIDTH;
      if ( coordinate_x + offset > resolution_width ) {
        // determine remaining spaces
        offset = coordinate_x + offset - resolution_width;
        // scroll if necessary
        scroll();
      }
      // apply offset
      coordinate_x += offset;
      break;

    default:
      // skip not supported characters
      if ( 127 < c ) {
        return;
      }
      // check whether line break is necessary
      if ( coordinate_x + FONT_WIDTH > resolution_width ) {
        scroll();
      }
      // max index
      int32_t end = FONT_HEIGHT * FONT_WIDTH;

      for( int32_t idx = 0; idx < end; idx++ ) {
        int32_t row = ( int32_t )( idx / FONT_WIDTH );
        int32_t column = idx % FONT_WIDTH;

        // determine color
        uint8_t set_color = ( font8x8_basic[ c ][ row ] & 1 << column )
          ? 0xff : 0;

        // put to frame buffer
        put_pixel(
          coordinate_x + column,
          coordinate_y + row,
          set_color,
          set_color,
          set_color
        );
      }
      // increase x coordinate
      coordinate_x += FONT_WIDTH;
  }
}
