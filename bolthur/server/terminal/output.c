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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "output.h"
#include "list.h"
#include "terminal.h"
#include "psf.h"
#include "render.h"
#include "../libterminal.h"
#include "../libframebuffer.h"

framebuffer_resolution_t resolution_data;

/**
 * @fn bool output_init(void)
 * @brief Generic output init
 *
 * @return
 *
 * @todo fetch framebuffer pid from vfs for rpc call
 */
bool output_init( void ) {
  // perform sync rpc call
  size_t response_info = _rpc_raise_wait(
    "#/dev/framebuffer#resolution", 5, NULL, 0
  );
  if ( errno ) {
    return false;
  }
  // fetch return
  _rpc_get_data( &resolution_data, sizeof( resolution_data ), response_info );
  return true;
}

/**
 * @fn void output_handle_out(pid_t, size_t)
 * @brief Handler for normal output stream
 *
 * @param origin
 * @param data_info
 */
void output_handle_out( __unused pid_t origin, size_t data_info ) {
  // handle no data
  if( ! data_info ) {
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    return;
  }
  // allocate for data fetching
  terminal_command_write_ptr_t terminal = malloc( sz );
  if ( ! terminal ) {
    return;
  }
  // fetch rpc data
  _rpc_get_data( terminal, sz, data_info );
  // handle error
  if ( errno ) {
    free( terminal );
    return;
  }
  // get terminal
  list_item_ptr_t found = list_lookup_data(
    terminal_list,
    terminal->terminal
  );
  if ( ! found ) {
    free( terminal );
    return;
  }
  // render
  render_terminal( found->data, terminal->data );
  // free terminal structure again
  free( terminal );
}

/**
 * @fn void output_handle_err(pid_t, size_t)
 * @brief Handler for error stream output
 *
 * @param origin
 * @param data_info
 */
void output_handle_err( __unused pid_t origin, size_t data_info ) {
  // handle no data
  if( ! data_info ) {
    return;
  }
  // get size for allocation
  size_t sz = _rpc_get_data_size( data_info );
  if ( errno ) {
    return;
  }
  // allocate for data fetching
  terminal_command_write_ptr_t terminal = malloc( sz );
  if ( ! terminal ) {
    return;
  }
  // fetch rpc data
  _rpc_get_data( terminal, sz, data_info );
  // handle error
  if ( errno ) {
    free( terminal );
    return;
  }
  // get terminal
  list_item_ptr_t found = list_lookup_data(
    terminal_list,
    terminal->terminal
  );
  if ( ! found ) {
    free( terminal );
    return;
  }
  // render
  render_terminal( found->data, terminal->data );
  // free terminal structure again
  free( terminal );
  // free all used temporary structures
  free( terminal );
}