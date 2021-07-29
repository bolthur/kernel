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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "core/helper.h"

#define FRAMEBUFFER_SCREEN_WIDTH 1024
#define FRAMEBUFFER_SCREEN_HEIGHT 768
#define FRAMEBUFFER_SCREEN_DEPTH 32

uint32_t width;
uint32_t height;
uint32_t pitch;
uint8_t* screen;

/**
 * @fn void put_pixel(uint32_t, uint32_t, uint32_t)
 * @brief Helper to put some pixel to screen
 *
 * @param x
 * @param y
 * @param color
 */
static void put_pixel( uint32_t x, uint32_t y, uint32_t color ) {
    uint32_t offs = ( y * pitch ) + ( x * 4 );
    *( ( uint32_t* )( screen + offs ) ) = color;
}

/**
 * @fn void draw_rect(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, int)
 * @brief Helper to draw a rect
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param color
 * @param fill
 */
static void draw_rect(
  uint32_t x1,
  uint32_t y1,
  uint32_t x2,
  uint32_t y2,
  uint32_t color,
  bool fill
) {
  for ( uint32_t y = y1; y <= y2; y++ ) {
    for ( uint32_t x = x1; x <= x2; x++ ) {
      if ( x == x1 || x == x2 || y == y1 || y == y2 || fill ) {
        put_pixel( x, y, color );
      }
    }
  }
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  pid_t pid = getpid();
  // print something
  EARLY_STARTUP_PRINT( "framebuffer init starting: %d!\r\n", pid )

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
    return 1;
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
  width = FRAMEBUFFER_SCREEN_WIDTH;
  height = FRAMEBUFFER_SCREEN_HEIGHT;
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
    return 1;
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
    return 1;
  }
  pitch = ( uint32_t )buffer[ 5 ];
  EARLY_STARTUP_PRINT( "pitch = %#lx\r\n", pitch )

  // draw
  draw_rect( 150, 150, 400, 400, 0xAA00AA, false );
  draw_rect( 450, 450, 500, 500, 0xAA00AA, true );

  // some output
  EARLY_STARTUP_PRINT( "-> pushing /dev/framebuffer to vfs!\r\n" )
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strcpy( msg->file_path, "/dev/framebuffer" );
  // perform add request
  send_add_request( msg );
  // print something
  EARLY_STARTUP_PRINT( "framebuffer init done!\r\n" )

  for(;;);
  return 0;
}
