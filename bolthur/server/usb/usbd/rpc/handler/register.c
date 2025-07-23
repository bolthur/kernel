/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
// driver includes
#include "../../usbd.h"
#include "../../../../libusb.h"

/**
 * @fn void rpc_handler_register(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler for device
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 *
 * @todo request real origin via syscall and check compare it to handler
 */
void rpc_handler_register(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    error.status = -EIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // allocate space for pull_request
  const usbd_register_device_handler_t* message = ( usbd_register_device_handler_t* )request->container;
  // validate against handler enum
  if (
    message->type < DEVICE_HANDLER_TYPE_DETACHED
    || message->type > DEVICE_HANDLER_TYPE_CHECK_CONNECTION
  ) {
    error.status = -EIO;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // try to find hub
  libusb_device_t* current = head;
  while ( current ) {
    // handle matching number
    if ( current->number == message->device_number ) {
      break;
    }
    // go to next
    current = current->next;
  }
  // handle not found
  if ( ! current ) {
    free( request );
    error.status = -ENXIO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // assert not set
  bool handler_not_yet_set = true;
  switch ( message->type ) {
    case DEVICE_HANDLER_TYPE_DETACHED:
      handler_not_yet_set = -1 == current->device_detached_handler;
      break;
    case DEVICE_HANDLER_TYPE_DEALLOCATE:
      handler_not_yet_set = -1 == current->device_deallocate_handler;
      break;
    case DEVICE_HANDLER_TYPE_CHECK_FOR_CHANGE:
      handler_not_yet_set = -1 == current->device_check_for_change_handler;
      break;
    case DEVICE_HANDLER_TYPE_CHILD_DETACHED:
      handler_not_yet_set = -1 == current->device_child_detached_handler;
      break;
    case DEVICE_HANDLER_TYPE_CHILD_RESET:
      handler_not_yet_set = -1 == current->device_child_reset_handler;
      break;
    case DEVICE_HANDLER_TYPE_CHECK_CONNECTION:
      handler_not_yet_set = -1 == current->device_check_connection_handler;
      break;
  }
  // handle already set
  if ( ! handler_not_yet_set ) {
    free( request );
    error.status = -EADDRINUSE;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // allocate response message
  const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( bool );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  memset( response, 0, response_size );
  // just set it to the handler
  switch ( message->type ) {
    case DEVICE_HANDLER_TYPE_DETACHED:
      handler_not_yet_set = message->handler;
      break;
    case DEVICE_HANDLER_TYPE_DEALLOCATE:
      handler_not_yet_set = message->handler;
      break;
    case DEVICE_HANDLER_TYPE_CHECK_FOR_CHANGE:
      handler_not_yet_set = message->handler;
      break;
    case DEVICE_HANDLER_TYPE_CHILD_DETACHED:
      handler_not_yet_set = message->handler;
      break;
    case DEVICE_HANDLER_TYPE_CHILD_RESET:
      handler_not_yet_set = message->handler;
      break;
    case DEVICE_HANDLER_TYPE_CHECK_CONNECTION:
      handler_not_yet_set = message->handler;
      break;
  }
  // return success
  response->status = 0;
  *( bool* )response->container = true;
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, 0 );
  free( request );
  free( response );
}
