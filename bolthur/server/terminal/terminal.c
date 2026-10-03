/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "terminal.h"
#include "output.h"
#include "../../library/collection/list/list.h"
#include "psf.h"
#include "main.h"
#include "global.h"
#include "../libconsole.h"
#include "../../library/vfs/dev.h"

list_manager_t* terminal_list = nullptr;
uint8_t* surface = nullptr;
framebuffer_surface_allocate_t surface_data = {
  .width = 0,
  .height = 0,
  .depth = 0,
  .shm_id = 0,
  .surface_id = 0,
  .pitch = 0,
};

/**
 * @fn int32_t terminal_lookup(const list_item_t*, const void*)
 * @brief List lookup helper
 * @param a
 * @param data
 * @return
 */
static int32_t terminal_lookup( const list_item_t* a, const void* data ) {
  return strcmp( ( ( terminal_t* )a->data )->path, data );
}

/**
 * @fn void terminal_cleanup(list_item_t*)
 * @brief List cleanup helper
 * @param a
 */
static void terminal_cleanup( list_item_t* a ) {
  auto const term = ( terminal_t* )a->data;
  // destroy circular buffer
  buffer_destroy( &term->buffer );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn bool terminal_allocate_framebuffer(void)
 * @brief Helper to allocate framebuffer
 * @returns
 */
bool terminal_allocate_framebuffer( void ) {
  // populate surface data
  surface_data.width = resolution_data.width;
  surface_data.height =  resolution_data.height;
  surface_data.depth = resolution_data.depth;
  // allocate surface
  const int result = ioctl(
    output_driver_fd,
    IOCTL_BUILD_REQUEST(
      FRAMEBUFFER_SURFACE_ALLOCATE,
      sizeof( surface_data ),
      IOCTL_RDWR
    ),
    &surface_data
  );
  // handle error
  if ( -1 == result ) {
    return false;
  }
  // try to allocate shared memory
  void* shm_addr = _syscall_memory_shared_attach( surface_data.shm_id, 0 );
  if ( errno ) {
    return false;
  }
  // populate surface
  surface = shm_addr;
  // return success
  return true;
}

/**
 * @fn bool terminal_init(void)
 * @brief Generic terminal init
 * @return
 */
bool terminal_init( void ) {
  // construct list
  terminal_list = list_construct( terminal_lookup, terminal_cleanup, nullptr );
  if ( ! terminal_list ) {
    return false;
  }
  console_command_add_t* command_add = malloc( sizeof( *command_add ) );
  if ( ! command_add ) {
    list_destruct( terminal_list );
    return false;
  }
  console_command_select_t* command_select = malloc( sizeof( *command_select ) );
  if ( ! command_select ) {
    free( command_add );
    list_destruct( terminal_list );
    return false;
  }
  // base path
  char *tty_path = malloc( sizeof( char ) * PATH_MAX );
  if ( ! tty_path ) {
    free( command_select );
    free( command_add );
    list_destruct( terminal_list );
    return false;
  }
  size_t in = TERMINAL_IN_START;
  size_t out = TERMINAL_OUT_START;
  size_t err = TERMINAL_ERR_START;
  // push terminals
  for ( uint32_t current = 0; current < TERMINAL_MAX_NUM; current++ ) {
    // prepare device path
    snprintf( tty_path, PATH_MAX, TERMINAL_BASE_PATH"%"PRIu32, current );
    // add device file
    const uint32_t device_info[] = { in, out, err, };
    if ( ! vfs_dev_add_file( tty_path, device_info, 3, nullptr ) ) {
      #if defined( TERMINAL_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
      #endif
      list_destruct( terminal_list );
      free( tty_path );
      free( command_add );
      free( command_select );
      free( terminal_list );
      return false;
    }
    // register handler for streams
    bolthur_rpc_bind( out, output_handle_out, false );
    if ( errno ) {
      #if defined( TERMINAL_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to bind rpc %zu: %s\r\n", out, strerror( errno ) )
      #endif
      list_destruct( terminal_list );
      free( tty_path );
      free( command_add );
      free( command_select );
      free( terminal_list );
      return false;
    }
    bolthur_rpc_bind( err, output_handle_err, false );
    if ( errno ) {
      #if defined( TERMINAL_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to bind rpc %zu: %s\r\n", err, strerror( errno ) )
      #endif
      list_destruct( terminal_list );
      free( tty_path );
      free( command_add );
      free( command_select );
      free( terminal_list );
      return false;
    }
    bolthur_rpc_bind( in, output_handle_in, false );
    if ( errno ) {
      #if defined( TERMINAL_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to bind rpc %zu: %s\r\n", in, strerror( errno ) )
      #endif
      list_destruct( terminal_list );
      free( tty_path );
      free( command_add );
      free( command_select );
      free( terminal_list );
      return false;
    }
    // allocate internal management structure
    terminal_t* term = malloc( sizeof( *term ) );
    if ( ! term ) {
      list_destruct( terminal_list );
      free( tty_path );
      free( command_add );
      free( command_select );
      free( terminal_list );
      return false;
    }
    // erase allocated space
    memset( term, 0, sizeof( *term ) );
    // push max columns and rows and tty path
    term->max_col = resolution_data.width / psf_glyph_width();
    term->max_row = resolution_data.height / psf_glyph_height();
    strncpy( term->path, tty_path, PATH_MAX - 1 );
    term->bpp = resolution_data.depth;
    // init ring buffer
    buffer_init( &term->buffer, term->max_row, term->max_col );
    // push back
    if ( ! list_push_back_data( terminal_list, term ) ) {
      free( tty_path );
      free( term );
      free( command_add );
      free( command_select );
      list_destruct( terminal_list );
      return false;
    }
    // erase
    memset( command_add, 0, sizeof( *command_add ) );
    // prepare structure
    strncpy( command_add->terminal, tty_path, PATH_MAX - 1 );
    command_add->in = in;
    command_add->out = out;
    command_add->err = err;
    command_add->origin = getpid();
    // call console add
    const int result = ioctl(
      console_manager_fd,
      IOCTL_BUILD_REQUEST(
        CONSOLE_ADD,
        sizeof( *command_add ),
        IOCTL_RDWR
      ),
      command_add
    );
    if ( -1 == result ) {
      free( tty_path );
      free( command_add );
      free( command_select );
      list_destruct( terminal_list );
      return false;
    }
    /// FIXME: WHAT ABOUT RETURN?
    //int response = *( ( int* )command_add );
    in += 3;
    out += 3;
    err += 3;
  }

  memset( command_select, 0, sizeof( *command_select ) );
  // prepare structure
  strncpy( command_select->path, "/dev/tty0", PATH_MAX - 1 );
  // call console select
  const int result = ioctl(
    console_manager_fd,
    IOCTL_BUILD_REQUEST(
      CONSOLE_SELECT,
      sizeof( *command_select ),
      IOCTL_RDWR
    ),
    command_select
  );
  if ( -1 == result ) {
    free( tty_path );
    free( command_add );
    free( command_select );
    list_destruct( terminal_list );
    return false;
  }
  const int response = *( ( int* )command_select );
  // free again
  free( tty_path );
  free( command_add );
  free( command_select );
  // return success
  return 0 == response;
}

/**
 * @fn const char* terminal_get_active(void)
 * @brief Get active terminal
 * @return
 */
char* terminal_get_active( void ) {
  // allocate space
  console_command_active_t* active = malloc( sizeof( *active ) );
  if ( ! active ) {
    return nullptr;
  }
  memset( active, 0, sizeof( *active ) );
  // call get active console
  const int result = ioctl(
    console_manager_fd,
    IOCTL_BUILD_REQUEST(
      CONSOLE_GET_ACTIVE,
      sizeof( *active ),
      IOCTL_RDWR
    ),
    active
  );
  // handle error
  if ( 0 != result ) {
    free( active );
    return nullptr;
  }
  // allocate memory
  char* path = malloc( sizeof( char ) * ( strlen( active->path ) + 1 ) );
  if ( ! path ) {
    free( active );
    return nullptr;
  }
  // clear and copy
  memset( path, 0, sizeof( char ) * ( strlen( active->path ) + 1 ) );
  strcpy( path, active->path );
  // free command
  free( active );
  // return path
  return path;
}
