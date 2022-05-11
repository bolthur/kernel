/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "collection/list.h"
#include "psf.h"
#include "utf8.h"
#include "main.h"
#include "../libconsole.h"
#include "../libhelper.h"

list_manager_ptr_t terminal_list;

/**
 * @fn int32_t terminal_lookup(const list_item_ptr_t, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t terminal_lookup(
  const list_item_ptr_t a,
  const void* data
) {
  return strcmp( ( ( terminal_ptr_t )a->data )->path, data );
}

/**
 * @fn void terminal_cleanup(const list_item_ptr_t)
 * @brief List cleanup helper
 *
 * @param a
 */
static void terminal_cleanup( const list_item_ptr_t a ) {
  terminal_ptr_t term = a->data;
  // detach shared memory
  if ( term->surface_memory_id ) {
    while ( true ) {
      _syscall_memory_shared_detach( term->surface_memory_id );
      if ( errno ) {
        continue;
      }
      break;
    }
  }
  /// FIXME: SEND DEALLOCATE TO FRAMEBUFFER
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn bool terminal_init(void)
 * @brief Generic terminal init
 *
 * @return
 */
bool terminal_init( void ) {
  // construct list
  terminal_list = list_construct( terminal_lookup, terminal_cleanup );
  if ( ! terminal_list ) {
    return false;
  }
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + sizeof( size_t ) * 3;
  vfs_add_request_ptr_t msg = malloc( msg_size );
  if ( ! msg ) {
    list_destruct( terminal_list );
    return false;
  }
  console_command_add_ptr_t command_add = malloc( sizeof( console_command_add_t ) );
  if ( ! command_add ) {
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  console_command_select_ptr_t command_select = malloc( sizeof( console_command_select_t ) );
  if ( ! command_select ) {
    free( command_add );
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  // base path
  char tty_path[ TERMINAL_MAX_PATH ];
  size_t in = RPC_CUSTOM_START;
  size_t out = RPC_CUSTOM_START + 1;
  size_t err = RPC_CUSTOM_START + 2;

  framebuffer_surface_allocate_t tmp = {
    .width = resolution_data.width,
    .height = resolution_data.height,
    .depth = resolution_data.depth,
    .shm_id = 0,
    .surface_id = 0,
  };
  // push terminals
  for ( uint32_t current = 0; current < TERMINAL_MAX_NUM; current++ ) {
    // prepare device path
    snprintf(
      tty_path,
      TERMINAL_MAX_PATH,
      TERMINAL_BASE_PATH"%"PRIu32,
      current
    );
    // clear memory
    memset( msg, 0, sizeof( vfs_add_request_t ) );
    // prepare message structure
    msg->info.st_mode = S_IFCHR;
    strncpy( msg->file_path, tty_path, PATH_MAX - 1 );
    msg->device_info[ 0 ] = in;
    msg->device_info[ 1 ] = out;
    msg->device_info[ 2 ] = err;
    // perform add request
    send_vfs_add_request( msg, msg_size, 0 );
    // register handler for streams
    bolthur_rpc_bind( out, output_handle_out );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to bind rpc %zu: %s\r\n",
        out,
        strerror( errno )
      )
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    bolthur_rpc_bind( err, output_handle_err );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to bind rpc %zu: %s\r\n",
        err,
        strerror( errno )
      )
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    bolthur_rpc_bind( in, output_handle_in );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to bind rpc %zu: %s\r\n",
        in,
        strerror( errno )
      )
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }

    // request surface from framebuffer
    tmp.surface_id = 0;
    tmp.shm_id = 0;
    int result = ioctl(
      output_driver_fd,
      IOCTL_BUILD_REQUEST(
        FRAMEBUFFER_SURFACE_ALLOCATE,
        sizeof( framebuffer_surface_allocate_t ),
        IOCTL_RDWR
      ),
      &tmp
    );
    if ( -1 == result ) {
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    // allocate internal management structure
    terminal_ptr_t term = malloc( sizeof( terminal_t ) );
    if ( ! term ) {
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    // erase allocated space
    memset( term, 0, sizeof( terminal_t ) );
    // push max columns and rows and tty path
    term->max_col = resolution_data.width / psf_glyph_width();
    term->max_row = resolution_data.height / psf_glyph_height();
    strncpy( term->path, tty_path, TERMINAL_MAX_PATH );
    term->bpp = resolution_data.depth;
    // try to allocate shared memory
    void* shm_addr = _syscall_memory_shared_attach( tmp.shm_id, 0 );
    if ( errno ) {
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    // fill into structure
    term->surface = shm_addr;
    term->surface_id = tmp.surface_id;
    term->surface_memory_id = tmp.shm_id;
    term->pitch = tmp.pitch;
    // push back
    if ( ! list_push_back( terminal_list, term ) ) {
      while ( true ) {
        _syscall_memory_shared_detach( tmp.shm_id );
        if ( errno ) {
          continue;
        }
        break;
      }
      free( term );
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    // erase
    memset( command_add, 0, sizeof( console_command_add_t ) );
    // prepare structure
    strncpy( command_add->terminal, tty_path, PATH_MAX - 1 );
    command_add->in = in;
    command_add->out = out;
    command_add->err = err;
    command_add->origin = getpid();
    // call console add
    result = ioctl(
      console_manager_fd,
      IOCTL_BUILD_REQUEST(
        CONSOLE_ADD,
        sizeof( console_command_add_t ),
        IOCTL_RDWR
      ),
      command_add
    );
    if ( -1 == result ) {
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    /// FIXME: WHAT ABOUT RETURN?
    //int response = *( ( int* )command_add );
    in += 3;
    out += 3;
    err += 3;
  }

  memset( command_select, 0, sizeof( console_command_select_t ) );
  // prepare structure
  strncpy( command_select->path, "/dev/tty0", PATH_MAX - 1 );
  // call console select
  int result = ioctl(
    console_manager_fd,
    IOCTL_BUILD_REQUEST(
      CONSOLE_SELECT,
      sizeof( console_command_select_t ),
      IOCTL_RDWR
    ),
    command_select
  );
  if ( -1 == result ) {
    free( command_add );
    free( command_select );
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  int response = *( ( int* )command_select );
  // free again
  free( msg );
  free( command_add );
  free( command_select );
  // return success
  return 0 == response;
}
