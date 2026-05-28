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
#include "../../hid.h"
#include "../../handler.h"
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../../libusbd.h"

/**
 * @fn void rpc_get_report(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler get report
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_get_report(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! request ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // allocate space for pull_request
  hid_get_report_t* message = ( hid_get_report_t* )request->container;
  // get device
  libusb_hid_device_t* dev;
  const int result = hid_get( message->device_number, &dev );
  if ( 0 != result ) {
    error.status = -result;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // handle report greater than report count
  if ( message->report > dev->parser_result->report_count ) {
    error.status = -EINVAL;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( message->shm_id, ( uintptr_t )NULL  );
  if ( errno ) {
    error.status = -errno;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // copy over parser
  const size_t initial_size = sizeof( libusb_hid_parser_report_t )
    + dev->parser_result->report[ message->report ]->fields_length * sizeof( libusb_hid_parser_fields_t );
  size_t offset = initial_size;
  memcpy( shm_addr, dev->parser_result->report[ message->report ], initial_size );
  // loop through report fields
  libusb_hid_parser_report_t* report_parser_to_return = shm_addr;
  for ( size_t i = 0; i < report_parser_to_return->fields_length; i++ ) {
    // skip variables or if no ptr is set
    if (
      report_parser_to_return->fields[ i ].attribute.variable
      || ! report_parser_to_return->fields[ i ].value.ptr
    ) {
      continue;
    }
    // calculate ptr size
    const size_t ptr_size = ( size_t )report_parser_to_return->fields[ i ].size * report_parser_to_return->fields[ i ].count / 8;
    // adjust ptr to local one
    auto void* new_ptr = ( void* )( ( uintptr_t )shm_addr + offset );
    // copy over stuff
    memcpy( new_ptr, report_parser_to_return->fields[ i ].value.ptr, ptr_size );
    // set relative new ptr
    report_parser_to_return->fields[ i ].value.ptr = ( void* )( ( uintptr_t )new_ptr - ( uintptr_t )shm_addr );
    #if defined( HID_ENABLE_DEBUG )
      STARTUP_PRINT( "i = %zu / %p\r\n", i, report_parser_to_return->fields[ i ].value.ptr )
    #endif
    // increment offset
    offset += ptr_size;
  }
  // calculate request size
  const size_t request_size = data_size - sizeof( vfs_ioctl_perform_request_t );
  // allocate response
  vfs_ioctl_perform_response_t* response = malloc(
    sizeof( vfs_ioctl_perform_response_t ) + request_size );
  // handle error
  if ( ! response ) {
    error.status = -ENOMEM;
    _syscall_memory_shared_detach( message->shm_id );
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL, 0 );
    return;
  }
  // populate response
  response->status = 0;
  memcpy( response->container, message, request_size );
  // return
  bolthur_rpc_return( RPC_VFS_IOCTL, response, request_size + sizeof( vfs_ioctl_perform_response_t ), NULL, 0 );
  // free request and response
  free( request );
  free( response );
}
