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
uint32_t pitch;
uint8_t* screen;

/**
 * @fn bool framebuffer_init(void)
 * @brief Init framebuffer
 *
 * @return
 */
bool framebuffer_init( void ) {
  // some output
  EARLY_STARTUP_PRINT( "-> initializing video driver!\r\n" )
  // mail buffer for following setup syscall
  int32_t buffer[ 256 ];

  // try to get display size
  buffer[ 0 ] = 8 * 4; // buffer size
  buffer[ 1 ] = 0; // perform request
  buffer[ 2 ] = 0x40003; // display size request
  buffer[ 3 ] = 8; // buffer size
  buffer[ 4 ] = 0; // request size
  buffer[ 5 ] = 0; // space for horizontal resolution
  buffer[ 6 ] = 0; // space for vertical resolution
  buffer[ 7 ] = 0; // closing tag
  // perform mailbox action
  _mailbox_action( buffer, sizeof( int32_t ) * 8 );
  // handle no valid return
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Mailbox error: %s\r\n", strerror( errno ) )
    return false;
  }
  // extract size
  width = ( uint32_t )buffer[ 5 ];
  height = ( uint32_t )buffer[ 6 ];
  EARLY_STARTUP_PRINT( "width = %ld, height = %ld\r\n", width, height )
  // Use fallback if not set
  if ( 0 == width && 0 == height ) {
    width = FRAMEBUFFER_SCREEN_WIDTH;
    height = FRAMEBUFFER_SCREEN_HEIGHT;
  }
  EARLY_STARTUP_PRINT( "width = %ld, height = %ld\r\n", width, height )

  // build request data for screen setup
  size_t idx = 1;
  buffer[ idx++ ] = 0; // request
  buffer[ idx++ ] = 0x00048003; // tag id (set physical size)
  buffer[ idx++ ] = 8; // value buffer size (bytes)
  buffer[ idx++ ] = 8; // request + value length (bytes)
  buffer[ idx++ ] = ( int32_t )width; // horizontal resolution
  buffer[ idx++ ] = ( int32_t )height; // vertical resolution
  buffer[ idx++ ] = 0x00048004; // tag id (set virtual size)
  buffer[ idx++ ] = 8; // value buffer size (bytes)
  buffer[ idx++ ] = 8; // request + value length (bytes)
  buffer[ idx++ ] = ( int32_t )width; // horizontal resolution
  buffer[ idx++ ] = ( int32_t )height; // vertical resolution
  buffer[ idx++ ] = 0x00048005; // tag id (set depth)
  buffer[ idx++ ] = 4; // value buffer size (bytes)
  buffer[ idx++ ] = 4; // request + value length (bytes)
  buffer[ idx++ ] = FRAMEBUFFER_SCREEN_DEPTH; // 32 bpp
  buffer[ idx++ ] = 0x00040001; // tag id (allocate framebuffer)
  buffer[ idx++ ] = 8; // value buffer size (bytes)
  buffer[ idx++ ] = 4; // request + value length (bytes)
  buffer[ idx++ ] = 16; // alignment = 16
  buffer[ idx++ ] = 0; // space for response
  buffer[ idx++ ] = 0; // closing tag
  buffer[ 0 ] = ( int32_t )idx * 4; // buffer size
  // perform mailbox action
  _mailbox_action( buffer, sizeof( int32_t ) * idx );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Mailbox error: %s\r\n", strerror( errno ) )
    return false;
  }
  EARLY_STARTUP_PRINT( "Allocated buffer at %p with size of %#lx\r\n",
    ( void* )buffer[ 19 ], buffer[ 20 ] )
  screen = ( uint8_t* )buffer[ 19 ];

  // get framebuffer pitch
  buffer[ 0 ] = 7 * 4; // total size
  buffer[ 1 ] = 0; // request
  buffer[ 2 ] = 0x40008; // display size
  buffer[ 3 ] = 4; // buffer size
  buffer[ 4 ] = 0; // request size
  buffer[ 5 ] = 0; // space for pitch
  buffer[ 6 ] = 0; // closing tag
  // perform mailbox action
  _mailbox_action( buffer, sizeof( int32_t ) * idx );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Mailbox error: %s\r\n", strerror( errno ) )
    return false;
  }
  pitch = ( uint32_t )buffer[ 5 ];
  EARLY_STARTUP_PRINT( "pitch = %#lx\r\n", pitch )

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
    EARLY_STARTUP_PRINT(
      "unable to register rpc handler: %s\r\n",
      strerror( errno )
    )
    return false;
  }
  _rpc_acquire(
    "#/dev/framebuffer#clear",
    ( uintptr_t )framebuffer_handle_clear
  );
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "unable to register rpc handler: %s\r\n",
      strerror( errno )
    )
    return false;
  }
  _rpc_acquire(
    "#/dev/framebuffer#render_text",
    ( uintptr_t )framebuffer_handle_render_text
  );
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "unable to register rpc handler: %s\r\n",
      strerror( errno )
    )
    return false;
  }
  return true;
}

/**
 * @fn void framebuffer_handle_resolution(pid_t, size_t)
 * @brief Handle resolution request currently only get
 *
 * @param origin
 * @param data_info
 */
void framebuffer_handle_resolution( pid_t origin, size_t data_info ) {
  // local variable for resolution data
  framebuffer_resolution_t resolution_data;
  memset( &resolution_data, 0, sizeof( framebuffer_resolution_t ) );
  resolution_data.success = -1;
  // startup
  EARLY_STARTUP_PRINT(
    "framebuffer_handle_resolution( %d, %d )\r\n",
    origin,
    data_info
  )
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "no data passed to command handler!\r\n" )
    _rpc_ret( &resolution_data, sizeof( framebuffer_resolution_t ) );
    return;
  }
  // allocate for data fetching
  framebuffer_command_ptr_t command = malloc(
    sizeof( framebuffer_command_t )
  );
  if ( ! command ) {
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    _rpc_ret( &resolution_data, sizeof( framebuffer_resolution_t ) );
    return;
  }
  // fetch rpc data
  _rpc_get_data( command, sizeof( framebuffer_command_t ), data_info );
  // handle error
  if ( errno ) {
    free( command );
    EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) )
    _rpc_ret( &resolution_data, sizeof( framebuffer_resolution_t ) );
    return;
  }
  // build return
  EARLY_STARTUP_PRINT( "Received resolution commant %d\r\n", command->command )
  resolution_data.success = 0;
  resolution_data.width = width;
  resolution_data.height = height;
  resolution_data.depth = FRAMEBUFFER_SCREEN_DEPTH;
  // free all used temporary structures
  free( command );
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
void framebuffer_handle_clear( pid_t origin, size_t data_info ) {
  // startup
  EARLY_STARTUP_PRINT(
    "framebuffer_handle_clear( %d, %d )\r\n",
    origin,
    data_info
  )
  // erase last line
  memset( screen, 0, width * height );
}

/**
 * @fn void framebuffer_handle_resolution(pid_t, size_t)
 * @brief Handle resolution request currently only get
 *
 * @param origin
 * @param data_info
 *
 * @todo return info with new x and y position
 */
void framebuffer_handle_render_text( pid_t origin, size_t data_info ) {
  // startup
  EARLY_STARTUP_PRINT(
    "framebuffer_handle_render_text( %d, %d )\r\n",
    origin,
    data_info
  )
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "no data passed to command handler!\r\n" )
    return;
  }
  // allocate for data fetching
  framebuffer_command_ptr_t command = malloc(
    sizeof( framebuffer_command_t )
  );
  if ( ! command ) {
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    return;
  }
  // fetch rpc data
  _rpc_get_data( command, sizeof( framebuffer_command_t ), data_info );
  // handle error
  if ( errno ) {
    free( command );
    EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) )
    return;
  }
  // build return
  EARLY_STARTUP_PRINT( "Render text command %d\r\n", command->command )
  EARLY_STARTUP_PRINT( "start_x = %"PRIu32"\r\n", command->text.start_x )
  EARLY_STARTUP_PRINT( "start_y = %"PRIu32"\r\n", command->text.start_y )
  EARLY_STARTUP_PRINT( "font_width = %"PRIu32"\r\n", command->text.font_width )
  EARLY_STARTUP_PRINT( "font_height = %"PRIu32"\r\n", command->text.font_height )
  EARLY_STARTUP_PRINT( "text = %s\r\n", command->text.text )
  // FIXME: Render text with freetype
  // free all used temporary structures
  free( command );
}
