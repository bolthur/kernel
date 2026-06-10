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

// system includes
#include <errno.h>
#include <stddef.h>
#include <assert.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
#include "../../libusbd.h"
// driver includes
#include "../../../../libusbd.h"

/**
 * @fn void rpc_attach_roothub_finished(size_t, pid_t, size_t, size_t)
 * @brief Attach roothub finished callback
 * @param type
 * @param origin
 * @param data_info
* @param response_info
 *
 * @todo add error retry
 */
static void rpc_attach_roothub_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "roothub finished\r\n" )
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  if ( ! async_data ) {
    EARLY_STARTUP_PRINT( "NO ASYNC DATA!\r\n" )
    _syscall_rpc_cleanup();
    return;
  }
  // get contexts
  const usbd_attach_context_t* ctx = async_data->context;
  assert( ctx );
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "roothub finished\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "roothub finished\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* attach_response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! attach_response ) {
    EARLY_STARTUP_PRINT( "roothub finished\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
    return;
  }
  EARLY_STARTUP_PRINT( "roothub finished\r\n" )
  // clear memory
  memset( &error, 0, sizeof( error ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), async_data, 0 );
}

/**
 * @fn void rpc_attach_roothub(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler for attaching roothub
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_attach_roothub(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // handle invalid origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "Invalid origin\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "roothub finished\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  char* dummy = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! dummy ) {
    EARLY_STARTUP_PRINT( "roothub finished\r\n" )
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle already attached
  if ( head ) {
    // free again
    free( dummy );
    EARLY_STARTUP_PRINT( "attach already done\r\n" )
    error.status = -EADDRINUSE;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching root hub\r\n" )
  #endif
  // try to attach root hub
  const int result = usbd_roothub_attach(
    rpc_attach_roothub_finished,
    origin,
    data_info,
    response_info,
    dummy,
    data_size
  );
  // free again
  free( dummy );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Attaching root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    error.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
}
