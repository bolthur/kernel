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

#include <sys/unistd.h>

#include "../libusbd.h"

/**
 * @fn libusb_device_t* usbd_get_root_hub(void)
 * @brief Wrapper to get root hub
 * @return
 */
libusb_device_t* usbd_roothub_get( void ) {
  // return first device or null if not set
  return head;
}

/**
 * @fn attach_roothub_finished( size_t, pid_t, size_t, size_t )
 * @brief Attach roothub finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void attach_roothub_finished(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  EARLY_STARTUP_PRINT( "ROOTHUB ATTACH FINISHED\r\n" )
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    USBD_ATTACH_ROOTHUB, response_info );
  // handle no async data
  if ( ! async_data ) {
    EARLY_STARTUP_PRINT( "NO ASYNC DATA\r\n" )
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // handle no data
  if ( ! data_info ) {
    EARLY_STARTUP_PRINT( "NO DATA\r\n" )
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    EARLY_STARTUP_PRINT( "INVALID ORIGIN\r\n" )
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    EARLY_STARTUP_PRINT( "NO DATA\r\n" )
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // handle result
  if ( 0 > response->status ) {
    EARLY_STARTUP_PRINT( "Attach of roothub failed: %s\r\n", strerror( -response->status ) )
  } else {
    EARLY_STARTUP_PRINT( "Roothub successfully attached\r\n" )
  }
  free( response );
  bolthur_rpc_destroy_async( async_data );
  _syscall_rpc_cleanup();
}

/**
 * @fn int usbd_roothub_attach(rpc_handler_t, pid_t, size_t, size_t)
 * @brief Wrapper to attach root hub
 * @param callback callback to be invoked
 * @param origin origin
 * @param data_info data info
 * @param response_info response info
 * @return 0 on success else errno
 */
int usbd_roothub_attach(
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Attaching root hob\r\n" )
  #endif
  // space for root hub
  libusb_device_t* root_hub = nullptr;
  // handle existing by freeing up
  if ( head && 1 == head->number ) {
    usbd_deallocate_device( head );
  }
  // allocate device
  int result = usbd_allocate_device( &root_hub, true );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Allocating root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // set device to powered on
  root_hub->status = LIBUSB_DEVICE_STATUS_POWERED;
  // attach usb device
  result = usbd_attach_device(
    root_hub,
    callback,
    origin,
    data_info,
    nullptr,
    0,
    response_info,
    true
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Attaching root hub failed: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_roothub_fire_attach
 * @brief Function to fire roothub attach
 * @return
 */
int usbd_roothub_fire_attach( void ) {
  // get current process id
  const pid_t target = getpid();
  char dummy;
  // raise rpc
  EARLY_STARTUP_PRINT( "RAISING RPC %d\r\n", USBD_ATTACH_ROOTHUB )
  const size_t response_id = bolthur_rpc_raise(
    USBD_ATTACH_ROOTHUB,
    target,
    &dummy,
    sizeof( dummy ),
    attach_roothub_finished,
    RPC_VFS_IOCTL,
    &dummy,
    sizeof( dummy ),
    0,
    0,
    nullptr,
    false
  );
  // return success depending on response id
  return 0 < response_id ? 0 : -1;
}
