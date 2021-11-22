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
#include "../../../libhelper.h"

uint32_t physical_width;
uint32_t physical_height;
uint32_t virtual_width;
uint32_t virtual_height;
uint32_t pitch;
uint32_t size;
uint32_t total_size;
uint8_t* screen;
uint8_t* current_back;
uint8_t* front_buffer;
uint8_t* back_buffer;

static int32_t mailbox_buffer[ 256 ];

struct framebuffer_rpc command_list[] = {
  {
    .command = FRAMEBUFFER_GET_RESOLUTION,
    .callback = framebuffer_handle_resolution
  }, {
    .command = FRAMEBUFFER_CLEAR,
    .callback = framebuffer_handle_clear
  }, {
    .command = FRAMEBUFFER_RENDER_SURFACE,
    .callback = framebuffer_handle_render_surface
  }, {
    .command = FRAMEBUFFER_FLIP,
    .callback = framebuffer_handle_flip
  },
};

/**
 * @fn bool framebuffer_init(void)
 * @brief Init framebuffer
 *
 * @return
 */
bool framebuffer_init( void ) {
  // try to get display size
  mailbox_buffer[ 0 ] = 0; //8 * sizeof( int32_t ); // buffer size
  mailbox_buffer[ 1 ] = 0; // perform request
  mailbox_buffer[ 2 ] = 0x40003; // display size request
  mailbox_buffer[ 3 ] = 8; // buffer size
  mailbox_buffer[ 4 ] = 0; // request size
  mailbox_buffer[ 5 ] = 0; // space for horizontal resolution
  mailbox_buffer[ 6 ] = 0; // space for vertical resolution
  // perform mailbox action
  _mailbox_action( mailbox_buffer, 8 );
  // handle no valid return
  if ( errno ) {
    return false;
  }
  // extract size
  physical_width = ( uint32_t )mailbox_buffer[ 5 ];
  physical_height = ( uint32_t )mailbox_buffer[ 6 ];
  // Use fallback if not set
  if ( 0 == physical_width && 0 == physical_height ) {
    physical_width = FRAMEBUFFER_SCREEN_WIDTH;
    physical_height = FRAMEBUFFER_SCREEN_HEIGHT;
  }

  // build request data for screen setup
  size_t idx = 1;
  mailbox_buffer[ idx++ ] = 0; // request
  // set physical size
  mailbox_buffer[ idx++ ] = 0x00048003;
  mailbox_buffer[ idx++ ] = 8; // value buffer size (bytes)
  mailbox_buffer[ idx++ ] = 8; // request + value length (bytes)
  mailbox_buffer[ idx++ ] = ( int32_t )physical_width; // horizontal resolution
  mailbox_buffer[ idx++ ] = ( int32_t )physical_height; // vertical resolution
  // set virtual size
  mailbox_buffer[ idx++ ] = 0x00048004;
  mailbox_buffer[ idx++ ] = 8; // value buffer size (bytes)
  mailbox_buffer[ idx++ ] = 8; // request + value length (bytes)
  // FIXME: width and height multiplied by 2 for qemu framebuffer flipping
  mailbox_buffer[ idx++ ] = ( int32_t )physical_width; // * 2; // horizontal resolution
  mailbox_buffer[ idx++ ] = ( int32_t )physical_height * 2; // vertical resolution
  // set depth
  mailbox_buffer[ idx++ ] = 0x00048005;
  mailbox_buffer[ idx++ ] = 4; // value buffer size (bytes)
  mailbox_buffer[ idx++ ] = 4; // request + value length (bytes)
  mailbox_buffer[ idx++ ] = FRAMEBUFFER_SCREEN_DEPTH; // 32 bpp
  // set virtual offset
  mailbox_buffer[ idx++ ] = 0x00048009;
  mailbox_buffer[ idx++ ] = 8; // value buffer size (bytes)
  mailbox_buffer[ idx++ ] = 8; // request + value length (bytes)
  mailbox_buffer[ idx++ ] = 0; // space for x
  mailbox_buffer[ idx++ ] = 0; // space for y
  // get pitch
  mailbox_buffer[ idx++ ] = 0x00040008;
  mailbox_buffer[ idx++ ] = 4; // buffer size
  mailbox_buffer[ idx++ ] = 0; // request size
  mailbox_buffer[ idx++ ] = 0; // space for pitch
  // set pixel order
  mailbox_buffer[ idx++ ] = 0x00048006;
  mailbox_buffer[ idx++ ] = 4; // value buffer size (bytes)
  mailbox_buffer[ idx++ ] = 4; // request + value length (bytes)
  mailbox_buffer[ idx++ ] = 0; // RGB encoding /// FIXME: CORRECT VALUE HERE?
  // allocate framebuffer
  mailbox_buffer[ idx++ ] = 0x00040001;
  mailbox_buffer[ idx++ ] = 8; // value buffer size (bytes)
  mailbox_buffer[ idx++ ] = 4; // request + value length (bytes)
  mailbox_buffer[ idx++ ] = 0x1000; // alignment = 0x1000
  mailbox_buffer[ idx++ ] = 0; // space for response

  // perform mailbox action
  _mailbox_action( mailbox_buffer, idx );
  if ( errno ) {
    return false;
  }

  // save screen information
  physical_width = ( uint32_t )mailbox_buffer[ 5 ];
  physical_height = ( uint32_t )mailbox_buffer[ 6 ];
  virtual_width = ( uint32_t )mailbox_buffer[ 10 ];
  virtual_height = ( uint32_t )mailbox_buffer[ 11 ];

  // save screen address as current and buffer size
  screen = ( uint8_t* )mailbox_buffer[ 32 ];
  total_size = ( uint32_t )mailbox_buffer[ 33 ];
  // save pitch
  pitch = ( uint32_t )mailbox_buffer[ 24 ];
  // set correct sizes
  size = pitch * physical_height;

  // clear everything after init completely
  memset( screen, 0, total_size );

  // set front and back buffers
  front_buffer = screen;
  back_buffer = ( uint8_t* )( screen + size );
  current_back = back_buffer;

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
  // register all handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // register rpc
    bolthur_rpc_bind( command_list[ i ].command, command_list[ i ].callback );
    if ( errno ) {
      return false;
    }
  }
  return true;
}

/**
 * @fn void framebuffer_flip(void)
 * @brief Framebuffer flip to back buffer
 */
void framebuffer_flip( void ) {/*
  // default request for screen flip
  static int32_t buffer[] = { 0, 0, 0x00048009, 0x8, 0, 0, 0 };
  // handle switch
  if ( screen == front_buffer ) {
    mailbox_buffer[ 6 ] = ( int32_t )physical_height;
    screen = back_buffer;
    current_back = front_buffer;
  } else {
    mailbox_buffer[ 6 ] = 0;
    screen = front_buffer;
    current_back = back_buffer;
  }
  // perform mailbox action
  _mailbox_action( buffer, 7 );
  // handle no valid return
  if ( errno ) {
    return;
  }
  // copy over screen into back buffer
  memcpy( current_back, screen, size );*/
  // copy over back buffer into screen
  memcpy( screen, current_back, size );
}

/**
 * @fn void framebuffer_handle_resolution(size_t, pid_t, size_t, size_t)
 * @brief Handle resolution request currently only get
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_resolution(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    return;
  }
  // local variable for resolution data
  framebuffer_resolution_t resolution_data;
  memset( &resolution_data, 0, sizeof( framebuffer_resolution_t ) );
  // build return
  resolution_data.success = 0;
  resolution_data.width = physical_width;
  resolution_data.height = physical_height;
  resolution_data.depth = FRAMEBUFFER_SCREEN_DEPTH;
  // return resolution data
  bolthur_rpc_return( RPC_VFS_IOCTL, &resolution_data, sizeof( resolution_data ), NULL );
}

/**
 * @fn void framebuffer_handle_clear(size_t, pid_t, size_t, size_t)
 * @brief Handle clear request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_clear(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    return;
  }
  memset( current_back, 0, size );
  framebuffer_flip();
}

/**
 * @fn void framebuffer_handle_render_surface(size_t, pid_t, size_t, size_t)
 * @brief RPC callback for rendering a surface
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_render_surface(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    return;
  }
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
  _rpc_get_data( info, sz, data_info, false );
  // handle error
  if ( errno ) {
    free( info );
    return;
  }
  // handle scroll up
  if ( info->scroll_y ) {
    // determine offset
    uint32_t offset = info->scroll_y * pitch;
    // move up and reset last line
    memmove( current_back, current_back + offset, size - offset );
    memset( current_back + size - offset, 0, offset );
  }
  // render stuff
  for ( uint32_t idx = 0; idx < info->max_y; idx += info->bpp ) {
    uint32_t y = idx / info->max_x;
    uint32_t x = idx % info->max_x;
    // calculate x and y values for rendering
    uint32_t final_x = info->x + ( x / info->bpp );
    uint32_t final_y = info->y + y;
    // extract color from data
    uint32_t color = *( ( uint32_t* )&info->data[ y * info->max_x + x ] );
    // determine render offset
    uint32_t offset = final_y * pitch + final_x * BYTE_PER_PIXEL;
    // push back color
    *( ( uint32_t* )( current_back + offset ) ) = color;
  }
  // free again
  free( info );
}

/**
 * @fn void framebuffer_handle_flip(size_t, pid_t, size_t, size_t)
 * @brief Handle flip request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_flip(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    return;
  }
  framebuffer_flip();
}
