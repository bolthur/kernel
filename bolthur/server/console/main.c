/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include <unistd.h>
#include <sys/bolthur.h>
#include "../libterminal.h"
#include "../libhelper.h"
#include "../libconsole.h"
#include "handler.h"
#include "../../library/collection/list/list.h"
#include "console.h"
#include "rpc.h"

list_manager_t* console_list = NULL;

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
int main( __unused int argc, __unused char* argv[] ) {
  EARLY_STARTUP_PRINT( "Bind specific vfs write request handler\r\n" )
  // set handler
  bolthur_rpc_bind( RPC_VFS_WRITE, rpc_handle_write, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler write!\r\n" )
    return -1;
  }
  // FIXME: SET READ HANDLER FOR STDIN

  console_list = list_construct( console_lookup, console_cleanup, NULL );
  if ( ! console_list ) {
    return -1;
  }
  EARLY_STARTUP_PRINT( "Setup rpc handler\r\n" )
  // register rpc handler
  if ( ! handler_register() ) {
    list_destruct( console_list );
    return -1;
  }

  // stdin device
  if ( !dev_add_file( "/dev/stdin", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }
  // stdout device
  if ( !dev_add_file( "/dev/stdout", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }
  // stderr device
  if ( !dev_add_file( "/dev/stderr", NULL, 0 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // console device
  uint32_t device_info[] = { CONSOLE_ADD, CONSOLE_SELECT, };
  if ( !dev_add_file( "/dev/console", device_info, 2 ) ) {
    EARLY_STARTUP_PRINT( "Unable to add dev fs\r\n" )
    return -1;
  }

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  // return exit code 0
  return 0;
}
