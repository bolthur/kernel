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
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>

#include "../libconsole.h"
#include "../libhelper.h"

pid_t pid = 0;

#define BASE_TERMINAL_PATH "/dev/tty"
#define MAX_TERMINAL_PATH 32
#define NUM_MAX_TERMINAL 7

#define CONSOLE_MANAGER "/dev/console"
#define OUTPUT_DRIVER "/dev/framebuffer"

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  assert( msg );
  console_command_ptr_t command = malloc( sizeof( console_command_t ) );
  assert( command );
  // print something
  EARLY_STARTUP_PRINT( "terminal processing!\r\n" )
  // cache current pid
  pid = getpid();

  // open /dev/console for writing
  int console_file = open( "/dev/console", O_WRONLY );
  if ( -1 == console_file ) {
    EARLY_STARTUP_PRINT( "Unable to open console device for writing: %s\r\n",
      strerror( errno ) )
    free( msg );
    return -1;
  }

  // base path
  char tty_path[ MAX_TERMINAL_PATH ];
  char in_path[ MAX_TERMINAL_PATH ];
  char out_path[ MAX_TERMINAL_PATH ];
  char err_path[ MAX_TERMINAL_PATH ];
  // push terminals
  for ( uint32_t current = 0; current < NUM_MAX_TERMINAL; current++ ) {
    // prepare device path
    snprintf(
      tty_path,
      MAX_TERMINAL_PATH,
      BASE_TERMINAL_PATH"%"PRIu32,
      current
    );
    snprintf(
      in_path,
      MAX_TERMINAL_PATH,
      "#"BASE_TERMINAL_PATH"%"PRIu32"#in",
      current
    );
    snprintf(
      out_path,
      MAX_TERMINAL_PATH,
      "#"BASE_TERMINAL_PATH"%"PRIu32"#out",
      current
    );
    snprintf(
      err_path,
      MAX_TERMINAL_PATH,
      "#"BASE_TERMINAL_PATH"%"PRIu32"#err",
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

    EARLY_STARTUP_PRINT( "-> preparing add command for console manager!\r\n" )
    // erase
    memset( command, 0, sizeof( console_command_t ) );
    // prepare structure
    command->command = CONSOLE_COMMAND_ADD;
    strncpy( command->add.terminal, tty_path, PATH_MAX );
    strncpy( command->add.in, in_path, PATH_MAX );
    strncpy( command->add.out, out_path, PATH_MAX );
    strncpy( command->add.err, err_path, PATH_MAX );
    // perform sync rpc call
    // FIXME: FETCH CONSOLE MANAGER PID FROM VFS
    size_t response_info = _rpc_raise_wait(
      "#/dev/console#add", 4, command, sizeof( console_command_t ) );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "unable to call rpc handler: %s\r\n",
        strerror( errno )
      )
    }
    if ( response_info ) {
      int response;
      _rpc_get_data( ( char* )&response, sizeof( int ), response_info );
      EARLY_STARTUP_PRINT( "add response = %d\r\n", response );
    }

    // FIXME: SEND ADD AND CHANGE STDOUT COMMANDS TO CONSOLE MANAGER
    // FIXME: USE SYNCHRONOUS REMOTE PROCEDURE CALL TO TALK TO CONSOLE MANAGER
    continue;
/*
    EARLY_STARTUP_PRINT( "-> preparing add command for console manager!\r\n" )
    // clear memory for add command
    memset( add, 0, sizeof( console_command_add_t ) );
    memset( container, 0, sizeof( console_command_container_t ) );
    // fill add command and push to container
    strncpy( add->path, tty_path, PATH_MAX );
    memcpy( container->data, add, sizeof( console_command_add_t ) );
    container->command = CONSOLE_COMMAND_ADD;

    // sending command via normal write
    EARLY_STARTUP_PRINT( "-> sending add command to console manager!\r\n" )
    // write to file
    if ( -1 == write(
      console_file,
      container,
      sizeof( console_command_container_t )
    ) ) {
      EARLY_STARTUP_PRINT(
        "Error while sending add command to console manager: %s\r\n",
        strerror( errno ) )
    }

    EARLY_STARTUP_PRINT( "-> preparing change command for console manager to change stdout!\r\n" )
    // clear memory for add command
    memset( change, 0, sizeof( console_command_change_t ) );
    memset( container, 0, sizeof( console_command_container_t ) );
    // fill add command and push to container
    strncpy( change->terminal_path, tty_path, PATH_MAX );
    strncpy( change->destination_path, OUTPUT_DRIVER, PATH_MAX );
    memcpy( container->data, change, sizeof( console_command_change_t ) );
    container->command = CONSOLE_COMMAND_CHANGE_STDOUT;

    // sending command via normal write
    EARLY_STARTUP_PRINT( "-> sending change command to console manager!\r\n" )
    // write to file
    if ( -1 == write(
      console_file,
      container,
      sizeof( console_command_container_t )
    ) ) {
      EARLY_STARTUP_PRINT(
        "Error while sending change command to console manager: %s\r\n",
        strerror( errno ) )
    }
*/
  }

  // push alias to current tty
  EARLY_STARTUP_PRINT( "-> pushing %s device to vfs!\r\n", BASE_TERMINAL_PATH )
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, BASE_TERMINAL_PATH, PATH_MAX );
  // perform add request
  send_add_request( msg );


  // push terminal device as indicator init is done
  EARLY_STARTUP_PRINT( "-> pushing /dev/terminal device to vfs!\r\n" )
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/terminal", PATH_MAX );
  // perform add request
  send_add_request( msg );

  // endless loop
  while( true ) {}
  // return exit code 0
  return 0;
}
