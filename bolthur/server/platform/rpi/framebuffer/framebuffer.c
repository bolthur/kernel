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
    .name = "resolution",
    .callback = ( uintptr_t )framebuffer_handle_resolution
  }, {
    .command = FRAMEBUFFER_CLEAR,
    .name = "clear",
    .callback = ( uintptr_t )framebuffer_handle_clear
  }, {
    .command = FRAMEBUFFER_RENDER_SURFACE,
    .name = "render_surface",
    .callback = ( uintptr_t )framebuffer_handle_render_surface
  }, {
    .command = FRAMEBUFFER_INFO,
    .name = "info",
    .callback = ( uintptr_t )framebuffer_handle_info
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
  char path[ PATH_MAX ];
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // erase local variable
    memset( path, 0, sizeof( char ) * PATH_MAX );
    // build path
    strncpy( path, "#/dev/framebuffer#", PATH_MAX );
    strncat( path, command_list[ i ].name, PATH_MAX - strlen( path ) );
    // register rpc
    _rpc_acquire( path, command_list[ i ].callback );
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
  resolution_data.width = physical_width;
  resolution_data.height = physical_height;
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
  int ret = 0;
  memset( current_back, 0, size );
  framebuffer_flip();
  _rpc_ret( &ret, sizeof( int ) );
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
  int ret = 0;
  // handle no data
  if( ! data_info ) {
    ret = -ENOMSG;
    _rpc_ret( &ret, sizeof( int ) );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    ret = -EIO;
    _rpc_ret( &ret, sizeof( int ) );
    return;
  }
  framebuffer_render_surface_ptr_t info = malloc( sz );
  if ( ! info ) {
    ret = -ENOMEM;
    _rpc_ret( &ret, sizeof( int ) );
    return;
  }
  // fetch rpc data
  _rpc_get_data( info, sz, data_info );
  // handle error
  if ( errno ) {
    ret = -EIO;
    _rpc_ret( &ret, sizeof( int ) );
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
      *( ( uint32_t* )( current_back + offset ) ) = color;
    }
  }
  // free again
  free( info );
  // flip it
  framebuffer_flip();
  _rpc_ret( &ret, sizeof( int ) );
}

/**
 * @fn void framebuffer_handle_info(pid_t, size_t)
 * @brief Handle ioctl info request
 *
 * @param origin
 * @param data_info
 */
void framebuffer_handle_info( __unused pid_t origin, size_t data_info ) {
  // dummy error response
  vfs_ioctl_info_response_t err_response = { .status = -EINVAL };
  // handle no data
  if( ! data_info ) {
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    return;
  }
  // allocate
  vfs_ioctl_info_request_ptr_t info = malloc( sz );
  if ( ! info ) {
    err_response.status = -ENOMEM;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    return;
  }
  // get request data
  memset( info, 0, sz );
  _rpc_get_data( info, sz, data_info );
  if ( errno ) {
    err_response.status = -EIO;
    _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
    free( info );
    return;
  }
  // max handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    if ( command_list[ i ].command == info->command ) {
      size_t response_size = sizeof( vfs_ioctl_info_response_t ) + (
        sizeof( char ) * ( strlen( command_list[ i ].name ) + 1 )
      );
      // allocate
      vfs_ioctl_info_response_ptr_t response = malloc( response_size );
      if ( ! response ) {
        err_response.status = -ENOMEM;
        _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
        free( info );
        return;
      }
      // fill response
      memset( response, 0, response_size );
      response->status = 0;
      strcpy( response->name, command_list[ i ].name );
      // return and exit
      _rpc_ret( response, response_size );
      free( info );
      free( response );
      return;
    }
  }
  free( info );
  err_response.status = -ENOSYS;
  _rpc_ret( &err_response, sizeof( vfs_ioctl_info_response_t ) );
}
