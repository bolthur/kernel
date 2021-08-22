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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "terminal.h"
#include "output.h"
#include "list.h"
#include "../libconsole.h"
#include "../libhelper.h"

list_manager_ptr_t terminal_list;

/**
 * @fn int32_t console_lookup(const list_item_ptr_t, const void*)
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
  terminal_ptr_t term = a->data;
  return strcmp( term->path, data );
}

/**
 * @fn void console_cleanup(const list_item_ptr_t)
 * @brief List cleanup helper
 *
 * @param a
 */
static void terminal_cleanup( const list_item_ptr_t a ) {
  terminal_ptr_t term = a->data;
  free( term->output_buffer );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn void terminal_init(void)
 * @brief Generic terminal init
 *
 * @todo fetch console manager pid from vfs for rpc call
 */
void terminal_init( void ) {
  // construct list
  terminal_list = list_construct( terminal_lookup, terminal_cleanup );
  assert( terminal_list );
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  console_command_ptr_t command = malloc( sizeof( console_command_t ) );
  assert( command );
  // base path
  char tty_path[ TERMINAL_MAX_PATH ];
  char in[ TERMINAL_MAX_PATH ];
  char out[ TERMINAL_MAX_PATH ];
  char err[ TERMINAL_MAX_PATH ];
  // push terminals
  for ( uint32_t current = 0; current < TERMINAL_MAX_NUM; current++ ) {
    // prepare device path
    snprintf(
      tty_path,
      TERMINAL_MAX_PATH,
      TERMINAL_BASE_PATH"%"PRIu32,
      current
    );
    snprintf(
      in,
      TERMINAL_MAX_PATH,
      "#"TERMINAL_BASE_PATH"%"PRIu32"#in",
      current
    );
    snprintf(
      out,
      TERMINAL_MAX_PATH,
      "#"TERMINAL_BASE_PATH"%"PRIu32"#out",
      current
    );
    snprintf(
      err,
      TERMINAL_MAX_PATH,
      "#"TERMINAL_BASE_PATH"%"PRIu32"#err",
      current
    );
    EARLY_STARTUP_PRINT( "-> pushing %s device to vfs!\r\n", tty_path )
    // clear memory
    memset( msg, 0, sizeof( vfs_add_request_t ) );
    // prepare message structure
    msg->info.st_mode = S_IFCHR;
    strncpy( msg->file_path, tty_path, PATH_MAX );
    // perform add request
    send_add_request( msg );


    // register handler for streams
    _rpc_acquire( out, ( uintptr_t )output_handle_out );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "unable to register rpc handler: %s\r\n",
        strerror( errno )
      )
    }
    _rpc_acquire( err, ( uintptr_t )output_handle_err );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "unable to register rpc handler: %s\r\n",
        strerror( errno )
      )
    }

    uint32_t max_x = resolution_data.width / OUTPUT_FONT_WIDTH;
    uint32_t max_y = resolution_data.height / OUTPUT_FONT_HEIGHT;
    EARLY_STARTUP_PRINT( "max_x = %"PRIu32", max_y = %"PRIu32"\r\n",
      max_x, max_y )
    size_t buffer_size = sizeof( uint8_t ) * max_x * max_y;
    EARLY_STARTUP_PRINT( "output buffer size = %#"PRIx32"\r\n",
      sizeof( uint8_t ) * max_x * max_y )
    // allocate internal management structure
    terminal_ptr_t term = malloc( sizeof( terminal_t ) );
    assert( term );
    // erase allocated space
    memset( term, 0, sizeof( terminal_t ) );
    // copy tty path and set max x/y
    strncpy( term->path, tty_path, TERMINAL_MAX_PATH );
    term->max_x = max_x;
    term->max_y = max_y;
    // allocate output buffer
    term->output_buffer = malloc( buffer_size );
    assert( term );
    assert( list_push_back( terminal_list, term ) );

    EARLY_STARTUP_PRINT( "-> preparing add command for console manager!\r\n" )
    // erase
    memset( command, 0, sizeof( console_command_t ) );
    // prepare structure
    command->command = CONSOLE_COMMAND_ADD;
    strncpy( command->add.terminal, tty_path, PATH_MAX );
    strncpy( command->add.in, in, PATH_MAX );
    strncpy( command->add.out, out, PATH_MAX );
    strncpy( command->add.err, err, PATH_MAX );
    // perform sync rpc call
    size_t response_info = _rpc_raise_wait(
      "#/dev/console#add", 4, command, sizeof( console_command_t ) );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "unable to call rpc handler: %s\r\n",
        strerror( errno )
      )
      continue;
    }
    int response;
    _rpc_get_data( ( char* )&response, sizeof( int ), response_info );
    EARLY_STARTUP_PRINT( "add response = %d\r\n", response );
  }

  EARLY_STARTUP_PRINT( "Selecting first console as active one!\r\n" )
  memset( command, 0, sizeof( console_command_t ) );
  // prepare structure
  command->command = CONSOLE_COMMAND_SELECT;
  strncpy( command->select.path, "/dev/tty0", PATH_MAX );
  // perform sync rpc call
  size_t response_info = _rpc_raise_wait(
    "#/dev/console#select", 4, command, sizeof( console_command_t ) );
  if ( errno ) {
    EARLY_STARTUP_PRINT(
      "unable to call rpc handler: %s\r\n",
      strerror( errno )
    )
    return;
  }
  int response;
  _rpc_get_data( ( char* )&response, sizeof( int ), response_info );
  EARLY_STARTUP_PRINT( "select response = %d\r\n", response );
}
