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

#include <sys/bolthur.h>
#include "../libterminal.h"
#include "../libconsole.h"
#include "../../library/collection/list/list.h"
#include "../../library/vfs/dev.h"
#include "console.h"
#include "handler.h"
#include "rpc.h"
#include "queue.h"
#include "global.h"

list_manager_t* console_list = nullptr;

/**
 * @fn int32_t console_lookup(const list_item_t*, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t console_lookup(
  const list_item_t* a,
  const void* data
) {
  console_t* console = a->data;
  return strcmp( console->path, data );
}

/**
 * @fn void console_cleanup(list_item_t*)
 * @brief List cleanup helper
 *
 * @param a
 */
static void console_cleanup( list_item_t* a ) {
  console_t* console = a->data;
  // destroy console
  console_destroy( console );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @brief main entry function
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // create console list
  console_list = list_construct( console_lookup, console_cleanup, nullptr );
  if ( ! console_list ) {
    return -1;
  }

  // setup handler
  #if defined( CONSOLE_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Setup handler tree\r\n" )
  #endif
  if ( ! handler_setup() ) {
    return -1;
  }

  // setup queue
  #if defined( CONSOLE_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Startup queue\r\n" )
  #endif
  if ( ! queue_setup() ) {
    return -1;
  }

  // register rpc handler
  #if defined( CONSOLE_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Setup rpc handler\r\n" )
  #endif
  if ( ! rpc_init() ) {
    list_destruct( console_list );
    return -1;
  }

  // stdin device
  if ( ! vfs_dev_add_file( "/dev/stdin", nullptr, 0, nullptr ) ) {
    #if defined( CONSOLE_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    return -1;
  }
  // stdout device
  if ( ! vfs_dev_add_file( "/dev/stdout", nullptr, 0, nullptr ) ) {
    #if defined( CONSOLE_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    return -1;
  }
  // stderr device
  if ( ! vfs_dev_add_file( "/dev/stderr", nullptr, 0, nullptr ) ) {
    #if defined( CONSOLE_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    return -1;
  }

  // enable rpc
  #if defined( CONSOLE_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  #endif
  _syscall_rpc_set_ready( true );

  // console device
  constexpr uint32_t device_info[] = {
    CONSOLE_ADD,
    CONSOLE_SELECT,
    CONSOLE_INPUT,
    RPC_VFS_IOCTL_TERMIOS_GET,
    RPC_VFS_IOCTL_TERMIOS_SET,
  };
  if ( ! vfs_dev_add_file( "/dev/console", device_info, 5, nullptr ) ) {
    #if defined( CONSOLE_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    #endif
    return -1;
  }

  // wait for rpc
  #if defined( CONSOLE_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  #endif
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
