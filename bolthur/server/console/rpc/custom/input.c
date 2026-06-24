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

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../../../libconsole.h"
#include "../../rpc.h"
#include "../../console.h"
#include "../../queue.h"
#include "../../../libterminal.h"

/**
 * @fn void rpc_handle_render_cleanup(size_t, pid_t, size_t, size_t)
 * @brief Async render cleanup
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void rpc_handle_render_cleanup(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // get matching async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_WRITE, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // handle no data
  if ( ! data_info ) {
    // cleanup
    _syscall_rpc_cleanup();
    bolthur_rpc_destroy_async( async_data );
    // skip rest
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_request_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    // cleanup
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // detach shared memory from original request
  const vfs_ioctl_perform_request_t* original_request = async_data->original_data;
  auto const terminal = ( terminal_write_request_t* )original_request->container;
  _syscall_memory_shared_detach( terminal->shm_id );
  bolthur_rpc_destroy_async( async_data );
  free( response );
  _syscall_rpc_cleanup();
}

/**
 * @fn static void render_character(console_t*, const char)
 * @brief Helper to render character to console
 * @param console
 * @param c
 */
static void render_character( console_t* console, const char c ) {
  if ( ! console ) {
    return;
  }
  constexpr size_t length = sizeof( char ) * 2;
  const size_t shm_id = _syscall_memory_shared_create( length );
  if ( errno ) {
    return;
  }
  char* shm = _syscall_memory_shared_attach( shm_id, 0 );
  if ( errno ) {
    return;
  }
  // build terminal command
  const size_t terminal_size = sizeof( terminal_write_request_t ) +
    sizeof( char ) * ( strlen( console->path ) + 1 );
  terminal_write_request_t* terminal = malloc( terminal_size );
  if ( ! terminal ) {
    _syscall_memory_shared_detach( shm_id );
    return;
  }
  // clear out memory
  memset( terminal, 0, terminal_size );
  // populate terminal
  terminal->len = length;
  terminal->shm_id = shm_id;
  strcpy( terminal->terminal, console->path );
  // handle not yet opened
  if ( 0 == console->fd ) {
    // open path
    const int fd = open( console->path, O_RDWR );
    // handle error
    if ( -1 == fd ) {
      _syscall_memory_shared_detach( shm_id );
      free( terminal );
      return;
    }
    // push back file handle
    console->fd = fd;
  }
  // populate shared memory
  shm[ 0 ] = c;
  shm[ 1 ] = '\0';
  // calculate rpc request size
  size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t );
  // add data to ioctl if existing
  rpc_request_size += terminal_size * sizeof( char );
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    _syscall_memory_shared_detach( shm_id );
    free( terminal );
    return;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = console->fd;
  rpc_request->command = console->out;
  rpc_request->type = IOCTL_RDWR;
  // push data if set / passed
  memcpy( rpc_request->container, terminal, terminal_size * sizeof( char ) );
  // route request to mount point
  bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    console->handler,
    rpc_request,
    rpc_request_size,
    rpc_handle_render_cleanup,
    RPC_VFS_WRITE,
    rpc_request,
    rpc_request_size,
    0,
    0,
    nullptr,
    false
    );
  // handle error
  if ( errno ) {
    _syscall_memory_shared_detach( shm_id );
    free( terminal );
    free( rpc_request );
    return;
  }
  // free request again
  free( rpc_request );
  free( terminal );
}

/**
 * @fn void rpc_custom_handle_input(size_t, pid_t, size_t, size_t)
 * @brief Console handle input command handler
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_handle_input(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate for data fetching
  auto const command = ( console_command_input_t* )request->container;
  // get active console and deactivate
  console_t* console = console_get_active();
  if ( ! console ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // set success flag and return before handling anything else
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
  // debug print buffer
  EARLY_STARTUP_PRINT( "%s\r\n", command->input );
  // handle input flags
  for ( size_t i = 0; i < CONSOLE_MAX_INPUT_SEQUENCE && command->input[ i ]; i++) {
    // cache character
    char c = command->input[ i ];
    // handle input flags
    if ( console->ios.c_iflag & ICRNL && c == '\r' ) {
      c = '\n';
    }

    // check for echo
    if ( console->ios.c_lflag & ECHO ) {
      render_character( console, c );
    }

    // cooked / raw handling
    if ( console->ios.c_lflag & ICANON ) {
      if ( '\b' == c || 0x7f == c ) {
        if ( console->raw.count > 0 ) {
          console->raw.head--; // delete last character
          console->raw.count--;
        }
        continue;
      }

      // push into raw
      console_buffer_push( &console->raw, c );

      // handle newline
      if ( '\n' == c || c == console->ios.c_cc[ VEOF ] ) {
        while ( console->raw.count > 0 ) {
          const char ch = console_buffer_pop( &console->raw );
          console_buffer_push( &console->cooked, ch );
        }
        // route to listening process
        queue_handle( "/dev/stdin", console->cooked.buffer );
      }
    } else {
      // raw mode
      console_buffer_push( &console->cooked, c );
      // route to listening process if minimum count was reached
      if ( console->cooked.count > console->ios.c_cc[ VMIN ] ) {
        queue_handle( "/dev/stdin", console->cooked.buffer );
      }
    }
  }
  // free all used temporary structures
  free( request );
}
