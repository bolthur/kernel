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
#include <assert.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "output.h"
#include "list.h"
#include "terminal.h"
#include "../libterminal.h"
#include "../libframebuffer.h"

framebuffer_resolution_t resolution_data;

/**
 * @fn void output_init(void)
 * @brief Generic output init
 *
 * @todo fetch framebuffer pid from vfs for rpc call
 */
void output_init( void ) {
  framebuffer_command_ptr_t command = malloc( sizeof( framebuffer_command_t ) );
  assert( command );
  EARLY_STARTUP_PRINT( "Fetching framebuffer data!\r\n" )
  memset( command, 0, sizeof( framebuffer_command_t ) );
  memset( &resolution_data, 0, sizeof( framebuffer_resolution_t ) );
  // prepare structure
  command->command = FRAMEBUFFER_GET_RESOLUTION;
  // perform sync rpc call
  size_t response_info = _rpc_raise_wait(
    "#/dev/framebuffer#resolution", 5,
    command, sizeof( framebuffer_command_t )
  );
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "unable to call rpc handler: %s\r\n",
      strerror( errno )
    )
    return;
  }
  // fetch return
  _rpc_get_data( &resolution_data, sizeof( resolution_data ), response_info );
  EARLY_STARTUP_PRINT(
    "width = %"PRIu32", height = %"PRIu32", depth = %"PRIu32", success = %"PRIi32"\r\n",
    resolution_data.width, resolution_data.height, resolution_data.depth,
    resolution_data.success
  );
}

/**
 * @fn void output_handle_out(pid_t, size_t)
 * @brief Handler for normal output stream
 *
 * @param origin
 * @param data_info
 */
void output_handle_out( pid_t origin, size_t data_info ) {
  EARLY_STARTUP_PRINT( "output_handle_out( %d, %d )\r\n", origin, data_info )
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "no data passed to command handler!\r\n" )
    return;
  }
  // allocate for data fetching
  terminal_command_ptr_t terminal = malloc( sizeof( terminal_command_t ) );
  if ( ! terminal ) {
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    return;
  }
  // fetch rpc data
  _rpc_get_data( terminal, sizeof( terminal_command_t ), data_info );
  // handle error
  if ( errno ) {
    free( terminal );
    EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) );
    return;
  }
  // FIXME: ADD HANDLING
  EARLY_STARTUP_PRINT( "Received output request\r\n" )
  EARLY_STARTUP_PRINT( "terminal = %s\r\n", terminal->write.terminal )
  EARLY_STARTUP_PRINT( "len = %#x\r\n", terminal->write.len )
  EARLY_STARTUP_PRINT( "data = %s\r\n", terminal->write.data )
  // get terminal
  list_item_ptr_t found = list_lookup_data(
    terminal_list,
    terminal->write.terminal
  );
  if ( ! found ) {
    free( terminal );
    EARLY_STARTUP_PRINT(
      "Terminal %s not handled here\r\n",
      terminal->write.terminal
    )
    return;
  }
  terminal_ptr_t term = found->data;
  // allocate data
  framebuffer_command_ptr_t render = malloc( sizeof( framebuffer_command_t ) );
  if ( ! render ) {
    free( terminal );
    EARLY_STARTUP_PRINT( "Unable to allocate text command\r\n" );
    return;
  }
  memset( render, 0, sizeof( framebuffer_command_t ) );
  render->command = FRAMEBUFFER_RENDER_TEXT;
  render->text.font_width = OUTPUT_FONT_WIDTH;
  render->text.font_height = OUTPUT_FONT_HEIGHT;
  render->text.start_x = term->current_x;
  render->text.start_y = term->current_y;
  strcpy( render->text.text, terminal->write.data );
  // raise without wait for return :)
  _rpc_raise(
    "#/dev/framebuffer#render_text", 5,
    render,
    sizeof( framebuffer_command_t )
  );
  // free all used temporary structures
  free( terminal );
  free( render );
}

/**
 * @fn void output_handle_err(pid_t, size_t)
 * @brief Handler for error stream output
 *
 * @param origin
 * @param data_info
 */
void output_handle_err( pid_t origin, size_t data_info ) {
  EARLY_STARTUP_PRINT( "output_handle_err( %d, %d )\r\n", origin, data_info )
  // handle no data
  if( ! data_info ) {
    EARLY_STARTUP_PRINT( "no data passed to command handler!\r\n" )
    return;
  }
  // allocate for data fetching
  terminal_command_ptr_t terminal = malloc( sizeof( terminal_command_t ) );
  if ( ! terminal ) {
    EARLY_STARTUP_PRINT( "malloc failed: %s\r\n", strerror( errno ) )
    return;
  }
  // fetch rpc data
  _rpc_get_data( terminal, sizeof( terminal_command_t ), data_info );
  // handle error
  if ( errno ) {
    free( terminal );
    EARLY_STARTUP_PRINT( "Fetch rpc data error: %s\r\n", strerror( errno ) );
    return;
  }
  // FIXME: ADD HANDLING
  EARLY_STARTUP_PRINT( "Received output request\r\n" )
  EARLY_STARTUP_PRINT( "terminal = %s\r\n", terminal->write.terminal )
  EARLY_STARTUP_PRINT( "len = %#x\r\n", terminal->write.len )
  EARLY_STARTUP_PRINT( "data = %s\r\n", terminal->write.data )
  // free all used temporary structures
  free( terminal );
}
