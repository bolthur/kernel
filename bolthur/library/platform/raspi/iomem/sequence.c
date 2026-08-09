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
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include "sequence.h"
#include "libiomem.h"

/**
 * @fn void* iomem_prepare_mmio_sequence(size_t, size_t*)
 * @brief Prepare mmio sequence
 * @param count amount of entries
 * @param total output variable for total size
 */
__attribute__((__malloc__, __malloc__(iomem_release_mmio_sequence, 1))) void* iomem_prepare_mmio_sequence( const size_t count, size_t* total ) {
  if ( 0 == count ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Invalid sequence count given\r\n" )
    #endif
    // return nullptr
    return nullptr;
  }
  // allocate
  const size_t tmp_total = count * sizeof( iomem_mmio_entry_t );
  iomem_mmio_entry_t* tmp = malloc( count * sizeof( iomem_mmio_entry_t ) );
  if ( ! tmp ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate sequence\r\n" )
    #endif
    // return nullptr
    return nullptr;
  }
  // erase
  memset( tmp, 0, tmp_total );
  // preset with defaults
  for ( uint32_t i = 0; i < count; i++ ) {
    tmp[ i ].shift_type = IOMEM_MMIO_SHIFT_NONE;
    tmp[ i ].sleep_type = IOMEM_MMIO_SLEEP_NONE;
    tmp[ i ].failure_condition = IOMEM_MMIO_FAILURE_CONDITION_OFF;
  }
  // set total if valid
  if ( total ) {
    *total = tmp_total;
  }
  // return
  return tmp;
}

/**
 * @fn void iomem_release_mmio_sequence(void*)
 * @brief Release iomem sequence
 * @param sequence
 */
void iomem_release_mmio_sequence( void* sequence ) {
  if ( sequence ) {
    free( sequence );
  }
}

/**
 * @fn int iomem_execute_sequence(int, void*, size_t)
 * @brief Function to execute a sequence
 * @param fd file descriptor
 * @param data data
 * @param size data size
 * @return
 */
int iomem_execute_sequence( const int fd, void* data, const size_t size ) {
  // allocate wrapper
  iomem_mmio_perform_t* perform = malloc( sizeof( *perform ) );
  // handle error
  if ( ! perform ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate wrapper structure\r\n" )
    #endif
    // return result
    return -1;
  }
  // clear out
  memset( perform, 0, sizeof( *perform ) );
  // allocate shared area
  const size_t shm_id = _syscall_memory_shared_create( size );
  if ( errno ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to allocate shared memory: %s\r\n",
        strerror( e ) )
    #endif
    // free wrapper
    free( perform );
    // return error
    return -1;
  }
  // attach shared area
  void* shm_addr = _syscall_memory_shared_attach( shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to attach shared memory: %s\r\n",
        strerror( e ) )
    #endif
    // free wrapper
    free( perform );
    // return error
    return -1;
  }
  // copy over
  memcpy( shm_addr, data, size );
  // populate wrapper
  perform->shm_id = shm_id;
  perform->length = size;
  // execute sequence
  const int result = ioctl(
    fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sizeof( *perform ),
      IOCTL_RDWR
    ),
    perform
  );
  // handle error
  if ( result != 0 ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to execute mmio sequence: %s\r\n",
        strerror( e ) )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // free wrapper
    free( perform );
    // return result
    return result;
  }
  // copy back result
  memcpy( data, shm_addr, size );
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // free wrapper
  free( perform );
  // return success
  return 0;
}

/**
 * @fn int iomem_execute_sequence_static(int, void*, size_t)
 * @brief Function to execute a sequence
 * @param fd file descriptor
 * @param data data
 * @param size data size
 * @return
 */
int iomem_execute_sequence_static( const int fd, void* data, const size_t size ) {
  // allocate shared area
  const size_t shm_id = _syscall_memory_shared_create( size );
  if ( errno ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to allocate shared memory: %s\r\n",
        strerror( e ) )
    #endif
    // return error
    return -1;
  }
  // attach shared area
  void* shm_addr = _syscall_memory_shared_attach( shm_id, ( uintptr_t )NULL );
  if ( errno ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to attach shared memory: %s\r\n",
        strerror( e ) )
    #endif
    // return error
    return -1;
  }
  // copy over
  memcpy( shm_addr, data, size );
  // populate wrapper
  iomem_mmio_perform_t perform = { .length = size, .shm_id = shm_id };
  // execute sequence
  const int result = ioctl(
    fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sizeof( perform ),
      IOCTL_RDWR
    ),
    &perform
  );
  // handle error
  if ( result != 0 ) {
    // error output
    #if defined( SEQUENCE_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Unable to execute mmio sequence: %s\r\n",
        strerror( e ) )
    #endif
    // detach shared memory
    _syscall_memory_shared_detach( shm_id );
    // return result
    return result;
  }
  // copy back result
  memcpy( data, shm_addr, size );
  // detach shared memory
  _syscall_memory_shared_detach( shm_id );
  // return success
  return 0;
}
