/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "framebuffer.h"
#include "../../../libframebuffer.h"

uint32_t width;
uint32_t height;
uint32_t physical_width;
uint32_t physical_height;
uint32_t pitch;
uint32_t size;
uint8_t* screen;
uint8_t* front_buffer;
uint8_t* back_buffer;

/**
 * @fn bool framebuffer_init(void)
 * @brief Init framebuffer
 *
 * @return
 */
bool framebuffer_init( void ) {
  // mail buffer for following setup syscall
  int32_t buf[ 256 ];

  // try to get display size
  buf[ 0 ] = 8 * 4; // buffer size
  buf[ 1 ] = 0; // perform request
  buf[ 2 ] = 0x40003; // display size request
  buf[ 3 ] = 8; // buffer size
  buf[ 4 ] = 0; // request size
  buf[ 5 ] = 0; // space for horizontal resolution
  buf[ 6 ] = 0; // space for vertical resolution
  buf[ 7 ] = 0; // closing tag
  // perform mailbox action
  _mailbox_action( buf, sizeof( int32_t ) * 8 );
  // handle no valid return
  if ( errno ) {
    return false;
  }
  // extract size
  width = ( uint32_t )buf[ 5 ];
  height = ( uint32_t )buf[ 6 ];
  // Use fallback if not set
  if ( 0 == width && 0 == height ) {
    width = FRAMEBUFFER_SCREEN_WIDTH;
    height = FRAMEBUFFER_SCREEN_HEIGHT;
  }

  // build request data for screen setup
  size_t idx = 1;
  buf[ idx++ ] = 0; // request
  buf[ idx++ ] = 0x00048003; // tag id (set physical size)
  buf[ idx++ ] = 8; // value buffer size (bytes)
  buf[ idx++ ] = 8; // request + value length (bytes)
  buf[ idx++ ] = ( int32_t )width; // horizontal resolution
  buf[ idx++ ] = ( int32_t )height; // vertical resolution
  buf[ idx++ ] = 0x00048004; // tag id (set virtual size)
  buf[ idx++ ] = 8; // value buffer size (bytes)
  buf[ idx++ ] = 8; // request + value length (bytes)
  buf[ idx++ ] = ( int32_t )width; // horizontal resolution
  buf[ idx++ ] = ( int32_t )height * 2; // vertical resolution
  buf[ idx++ ] = 0x00048005; // tag id (set depth)
  buf[ idx++ ] = 4; // value buffer size (bytes)
  buf[ idx++ ] = 4; // request + value length (bytes)
  buf[ idx++ ] = FRAMEBUFFER_SCREEN_DEPTH; // 32 bpp
  buf[ idx++ ] = 0x00040001; // tag id (allocate framebuffer)
  buf[ idx++ ] = 8; // value buffer size (bytes)
  buf[ idx++ ] = 4; // request + value length (bytes)
  buf[ idx++ ] = 16; // alignment = 16
  buf[ idx++ ] = 0; // space for response
  buf[ idx++ ] = 0; // closing tag
  buf[ 0 ] = ( int32_t )idx * 4; // buffer size
  // perform mailbox action
  _mailbox_action( buf, sizeof( int32_t ) * idx );
  if ( errno ) {
    return false;
  }

  // extract virtual and physical sizes
  width = ( uint32_t )buf[ 10 ];
  height = ( uint32_t )buf[ 11 ];
  physical_width = ( uint32_t )buf[ 5 ];
  physical_height = ( uint32_t )buf[ 6 ];
  // save screen addressas current and buffer size
  screen = ( uint8_t* )buf[ 19 ];
  size = ( uint32_t )buf[ 20 ];

  // get framebuffer pitch
  buf[ 0 ] = 7 * 4; // total size
  buf[ 1 ] = 0; // request
  buf[ 2 ] = 0x40008; // display size
  buf[ 3 ] = 4; // buffer size
  buf[ 4 ] = 0; // request size
  buf[ 5 ] = 0; // space for pitch
  buf[ 6 ] = 0; // closing tag
  // perform mailbox action
  _mailbox_action( buf, sizeof( int32_t ) * idx );
  if ( errno ) {
    return false;
  }
  pitch = ( uint32_t )buf[ 5 ];

  // set front and back buffers
  front_buffer = screen;
  back_buffer = ( uint8_t* )( ( uintptr_t )screen + pitch * physical_height );

  // return with register of necessary rpc
  return framebuffer_register_rpc();
}

/**
 * @fn bool framebuffer_register_rpc(void)
 * @brief Register necessary rpc handler
 *
 * @return
 */
bool framebuffer_register_rpc( void ) {
  _rpc_acquire(
    "#/dev/framebuffer#resolution",
    ( uintptr_t )framebuffer_handle_resolution
  );
  if ( errno ) {
    return false;
  }
  _rpc_acquire(
    "#/dev/framebuffer#clear",
    ( uintptr_t )framebuffer_handle_clear
  );
  if ( errno ) {
    return false;
  }
  _rpc_acquire(
    "#/dev/framebuffer#render_surface",
    ( uintptr_t )framebuffer_handle_render_surface
  );
  if ( errno ) {
    return false;
  }
  return true;
}

/**
 * @fn void framebuffer_flip(void)
 * @brief Framebuffer flip to back buffer
 */
void framebuffer_flip( void ) {
  // mail buffer for following setup syscall
  int32_t buf[ 256 ];
  uint8_t* new_front_buffer = front_buffer;
  uint8_t* new_back_buffer = back_buffer;
  // build base request
  buf[ 0 ] = 8 * 4; // buffer size
  buf[ 1 ] = 0; // perform request
  buf[ 2 ] = 0x48009; // display size request
  buf[ 3 ] = 8; // buffer size
  buf[ 4 ] = 0; // request size
  buf[ 5 ] = 0; // space for x position
  buf[ 6 ] = 0; // space for y position
  buf[ 7 ] = 0; // closing tag
  // handle switch back
  if ( screen == front_buffer ) {
    buf[ 6 ] = ( int32_t )height;
    new_front_buffer = back_buffer;
    new_back_buffer = front_buffer;
  }
  // perform mailbox action
  _mailbox_action( buf, sizeof( int32_t ) * 8 );
  // handle no valid return
  if ( errno ) {
    return;
  }
  // set screen to new buffers
  screen = new_front_buffer;
  front_buffer = new_front_buffer;
  back_buffer = new_back_buffer;
}

/**
 * @fn void framebuffer_handle_resolution(pid_t, size_t)
 * @brief Handle resolution request currently only get
 *
 * @param origin
 * @param data_info
 */
void framebuffer_handle_resolution(
  __unused pid_t origin,
  __unused size_t data_info
) {
  // local variable for resolution data
  framebuffer_resolution_t resolution_data;
  memset( &resolution_data, 0, sizeof( framebuffer_resolution_t ) );
  resolution_data.success = -1;
  // build return
  resolution_data.success = 0;
  resolution_data.width = width;
  resolution_data.height = height;
  resolution_data.depth = FRAMEBUFFER_SCREEN_DEPTH;
  // return resolution data
  _rpc_ret( &resolution_data, sizeof( framebuffer_resolution_t ) );
}

/**
 * @fn void framebuffer_handle_clear(pid_t, size_t)
 * @brief Handle clear request
 *
 * @param origin
 * @param data_info
 */
void framebuffer_handle_clear(
  __unused pid_t origin,
  __unused size_t data_info
) {
  memset( back_buffer, 0, size );
  framebuffer_flip();
}

/**
 * @fn void framebuffer_handle_render_surface(pid_t, size_t)
 * @brief RPC callback for rendering a surface
 *
 * @param origin
 * @param data_info
 */
void framebuffer_handle_render_surface(
  __unused pid_t origin,
  size_t data_info
) {
  // handle no data
  if( ! data_info ) {
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    return;
  }
  framebuffer_render_surface_ptr_t info = malloc( sz );
  if ( ! info ) {
    return;
  }
  // fetch rpc data
  _rpc_get_data( info, sz, data_info );
  // handle error
  if ( errno ) {
    free( info );
    return;
  }
  for ( uint32_t y = 0; y < info->max_y / info->max_x; y++ ) {
    for ( uint32_t x = 0; x < info->max_x; x += BYTE_PER_PIXEL ) {
      // calculate x and y values for rendering
      uint32_t final_x = info->x + ( x / BYTE_PER_PIXEL );
      uint32_t final_y = info->y + y;
      // extract color from data
      uint32_t color = *( ( uint32_t* )&info->data[ y * info->max_x + x ] );
      // determine render offset
      uint32_t offset = final_y * pitch + final_x * BYTE_PER_PIXEL;
      // push back color
      *( ( uint32_t* )( back_buffer + offset ) ) = color;
    }
  }
  // free again
  free( info );
  // flip it
  framebuffer_flip();
}
