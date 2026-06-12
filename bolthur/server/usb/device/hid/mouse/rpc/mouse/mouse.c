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
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "../../global.h"
#include "../../rpc.h"
#include "../../mouse.h"
#include "../../../../../../libusbd.h"

/**
 * @fn void rpc_keyboard_key(size_t, pid_t, size_t, size_t)
 * @brief Key rpc handler callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo add proper error handling
 */
void rpc_mouse_mouse(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // get async data
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info );
  // get original interrupt message
  const usbd_interrupt_message_t* original_interrupt_message = nullptr;
  if ( async_data ) {
    const vfs_ioctl_perform_request_t* original_request = async_data->original_data;
    original_interrupt_message = ( usbd_interrupt_message_t* )original_request->container;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, NULL );
  if ( ! response ) {
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // handle enumerating
  if ( response->status == -EAGAIN ) {
    // handle valid original interrupt message
    if ( original_interrupt_message ) {
      // attach shared memory
      void* shm_addr = _syscall_memory_shared_attach( original_interrupt_message->shm_id, ( uintptr_t )NULL );
      if ( errno ) {
        free( response );
        _syscall_rpc_cleanup();
        return;
      }
      // transform shared memory into message
      const usb_interrupt_poll_t* message = ( usb_interrupt_poll_t* )shm_addr;
      // try to get device by number
      libusb_mouse_device_t* dev = mouse_get_device( message->device_number );
      // handle no device found
      if ( ! dev ) {
        _syscall_memory_shared_detach( original_interrupt_message->shm_id );
        free( response );
        _syscall_rpc_cleanup();
        return;
      }
      // detach shared memory if existing
      _syscall_memory_shared_detach( original_interrupt_message->shm_id );
      dev->running_poll = 0;
    }
    free( response );
    bolthur_rpc_destroy_async( async_data );
    _syscall_rpc_cleanup();
    return;
  }
  // destroy it if existing
  if ( async_data ) {
    bolthur_rpc_destroy_async( async_data );
  }
  // get message
  const usbd_interrupt_message_t* control_message = ( usbd_interrupt_message_t* )response->container;
  // attach shared memory
  void* shm_addr = _syscall_memory_shared_attach( control_message->shm_id, ( uintptr_t )NULL );
  // handle error
  if ( errno ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // transform shared memory into message
  const usb_interrupt_poll_t* message = ( usb_interrupt_poll_t* )shm_addr;
  // try to get device by number
  libusb_mouse_device_t* dev = mouse_get_device( message->device_number );
  // handle no device found
  if ( ! dev ) {
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  dev->last_usb_pid = message->last_usb_pid;
  dev->last_packet_count = message->last_packet_transfer;
  // handle error
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // handle stall by clearing stall bit
    if ( message->error & LIBUSB_TRANSFER_ERROR_STALL ) {
      /// FIXME: IMPLEMENT STALL RESET
      dev->running_poll = 0;
    // handle nack ( nothing there ) by just resetting running poll
    } else if ( message->error & LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE ) {
      dev->running_poll = 0;
    }
    // cleanup everything and return
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle not enough transferred
  if ( message->last_transfer != MOUSE_REPORT_SIZE ) {
    _syscall_memory_shared_detach( control_message->shm_id );
    free( response );
    dev->running_poll = 0;
    _syscall_rpc_cleanup();
    return;
  }
  /// FIXME: IMPLEMENT
  // cleanup everything and return
  _syscall_memory_shared_detach( control_message->shm_id );
  free( response );
  _syscall_rpc_cleanup();
}
