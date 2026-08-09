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

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "random.h"
#include "../../../../library/platform/raspi/iomem/libperipheral.h"
#include "../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../library/platform/raspi/iomem/sequence.h"

int iomem_fd;

#include <stdarg.h>
/**
 * @fn int ioctl(int, uint64_t, ...)
 * @brief ioctl implementation
 *
 * @param file file descriptor
 * @param request request information ( command and data size combined )
 * @param ... optional third parameter for data
 * @return
 *
 * @todo add reentrance safety
 */
__weak_symbol __attribute__((__optimize__("O0"))) int ioctl( int file, uint64_t request, ... ) {
  // extract size and request from request ( 16 bit each )
  const uint32_t command = IOCTL_REQUEST_GET_COMMAND( request );
  const uint32_t data_size = IOCTL_REQUEST_GET_SIZE( request );
  const uint32_t type = IOCTL_REQUEST_GET_TYPE( request );
  // extract possible data parameter
  va_list list;
  va_start( list, request );
  void* data = ( void* )va_arg( list, void* );
  va_end( list );
  // handle invalid data
  if ( ! data && data_size ) {
    errno = EINVAL;
    return -1;
  }
  // calculate rpc request size
  size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t );
  // add data to ioctl if existing
  if ( data && data_size && IOCTL_NONE != type ) {
    rpc_request_size += data_size * sizeof( char );
  }
  // allocate rpc structures
  vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
  if ( ! rpc_request ) {
    errno = ENOMEM;
    return -1;
  }
  // clear rpc structures
  memset( rpc_request, 0, rpc_request_size );
  // populate structure
  rpc_request->handle = file;
  rpc_request->command = command;
  rpc_request->type = type;
  // push data if set / passed
  if ( data && data_size && IOCTL_NONE != type ) {
    memcpy( rpc_request->container, data, data_size * sizeof( char ) );
  }
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_IOCTL,
    VFS_DAEMON_ID,
    rpc_request,
    rpc_request_size,
    nullptr,
    RPC_VFS_IOCTL,
    rpc_request,
    rpc_request_size,
    0,
    0,
    nullptr,
    false
  );
  if ( ! response_id ) {
    // free request data
    free( rpc_request );
    return -1;
  }
  // free request data
  free( rpc_request );
  // get response from mailbox
  size_t rpc_response_size;
  vfs_ioctl_perform_response_t* rpc_response = bolthur_rpc_fetch_from_mailbox(
    response_id,
    &rpc_response_size,
    true,
    nullptr
  );
  // handle error
  if ( ! rpc_response ) {
    errno = ENOMSG;
    return -1;
  }
  // save response status before free
  const int status = rpc_response->status;
  if ( 0 > status ) {
    free( rpc_response );
    errno = -status;
    return -1;
  }
  const uint32_t response_data_size = rpc_response_size - sizeof( vfs_ioctl_perform_response_t );
  // copy response data if set
  if ( data && data_size && IOCTL_NONE != type ) {
    // handle no data with clear
    if ( 0 == response_data_size ) {
      memset( data, 0, data_size );
    // handle data size to small
    } else if ( data_size < response_data_size ) {
      EARLY_STARTUP_PRINT( "data_size = %"PRIu32", response_data_size = %"PRIu32"\r\n",
        data_size, response_data_size );
      free( rpc_response );
      errno = ENOMEM;
      return -1;
    // copy over content
    } else {
      memcpy( data, rpc_response->container, response_data_size );
    }
  }
  // free stuff and return
  free( rpc_response );
  return status;
}

/**
 * @fn bool random_setup(void)
 * @brief Prepare random chip
 *
 * @return
 */
bool random_setup( void ) {
  // open device
  iomem_fd = open( IOMEM_DEVICE_PATH, O_RDWR );
  // handle error
  if ( -1 == iomem_fd ) {
    return false;
  }
  // build write data
  iomem_mmio_entry_t data[] = {
    // read interrupt
    {
      .type = IOMEM_MMIO_ACTION_READ,
      .offset = PERIPHERAL_RNG_INTERRUPT,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // mask interrupt
    {
      .type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ,
      .offset = PERIPHERAL_RNG_INTERRUPT,
      .value = 0x1,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // rng warmup count
    {
      .type = IOMEM_MMIO_ACTION_WRITE,
      .offset = PERIPHERAL_RNG_STATUS,
      .value = 0x40000,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // enable
    {
      .type = IOMEM_MMIO_ACTION_WRITE,
      .offset = PERIPHERAL_RNG_CONTROL,
      .value = 0x1,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
  };
  // perform ioctl
  if ( -1 == iomem_execute_sequence_static( iomem_fd, data, sizeof( data ) ) ) {
    errno = ENOSYS;
    close( iomem_fd );
    return false;
  }
  // return success
  return true;
}

/**
 * @fn uint32_t random_generate_number(void)
 * @brief Returns generated random number
 *
 * @return
 */
uint32_t random_generate_number( void ) {
  // build write data
  iomem_mmio_entry_t data[] = {
    // loop until rng is ready
    {
      .type = IOMEM_MMIO_ACTION_LOOP_FALSE,
      .offset = PERIPHERAL_RNG_STATUS,
      .shift_type = IOMEM_MMIO_SHIFT_RIGHT,
      .shift_value = 24,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
    // read data
    {
      .type = IOMEM_MMIO_ACTION_READ,
      .offset = PERIPHERAL_RNG_DATA,
      .shift_type = IOMEM_MMIO_SHIFT_NONE,
      .sleep_type = IOMEM_MMIO_SLEEP_NONE,
    },
  };
  // perform request
  if ( -1 == iomem_execute_sequence_static( iomem_fd, data, sizeof( data ) ) ) {
    errno = EIO;
    return 0;
  }
  // return read value
  return data[ 1 ].value;
}
