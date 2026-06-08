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
#include <inttypes.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/hid/hid.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_mouse_attach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler attach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_mouse_attach(
  size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( ! request ) {
    err_response.status = -ENOMSG;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;
  // get hid driver
  uint32_t device_driver;
  int result = hid_get_driver( message->device_number, &device_driver );
  if ( 0 != result ) {
    free( request );
    err_response.status = -result;
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // handle invalid device driver
  if ( device_driver != DEVICE_DRIVER_HID ) {
    free( request );
    bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  /// FIXME: IMPLEMENT
  STARTUP_PRINT( "MOUSE ATTACH FOLLOWING!\r\n" )
  free( request );
  memset( &err_response, 0, sizeof( err_response ) );
  bolthur_rpc_return( type, &err_response, sizeof( err_response ), nullptr, 0 );
}
