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

#include <errno.h>
#include "../../rpc.h"
#include "../../libusbd.h"

/**
 * @fn void rpc_interrupt_generic( size_t, pid_t, size_t, size_t )
 * @brief Interrupt generic notification endpoint
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_interrupt_generic(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    _syscall_rpc_cleanup();
    return;
  }
  // handle no data
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get message
  auto const message = ( usbd_interrupt_return_t* )response->container;
  // find device
  libusb_device_t* device;
  const int result = usbd_device_get_by_number( message->device_number, &device );
  if ( 0 != result ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // call poll origin with fire and forget
  bolthur_rpc_raise_generic(
    GENERIC_POLL_INTERRUPT,
    device->poll_origin,
    response,
    data_size,
    nullptr,
    GENERIC_POLL_INTERRUPT,
    nullptr,
    0,
    0,
    0,
    nullptr,
    true,
    true
  );
  // free up and return
  free( response );
  _syscall_rpc_cleanup();
}
