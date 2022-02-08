/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../emmc.h"
// from iomem
#include "../../../libemmc.h"
#include "../../../libiomem.h"
#include "../../../libperipheral.h"
#include "../../../libmailbox.h"

/**
 * @fn void prepare_mailbox*(size_t, size_t*)
 * @brief Helper to allocate and clear mailbox property array
 *
 * @param count
 * @param total
 */
static void* prepare_mailbox( size_t count, size_t* total ) {
  if ( 0 == count ) {
    return NULL;
  }
  // allocate
  size_t tmp_total = count * sizeof( uint32_t );
  iomem_mmio_entry_ptr_t tmp = malloc( tmp_total );
  if ( ! tmp ) {
    return NULL;
  }
  // erase
  memset( tmp, 0, tmp_total );
  // set total if not null
  if ( total ) {
    *total = tmp_total;
  }
  // return
  return tmp;
}

/**
 * @fn emmc_response_t emmc_controller_shutdown(void)
 * @brief Shutdown emmc controller by using mailbox interface
 *
 * @return
 */
emmc_response_t emmc_controller_shutdown( void ) {
  // allocate buffer
  size_t request_size;
  int32_t* request = prepare_mailbox( 8, &request_size );
  if ( ! request ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // build request
  // buffer size
  request[ 0 ] = ( int32_t )request_size;
  // perform request
  request[ 1 ] = 0;
  // power state tag
  request[ 2 ] = MAILBOX_SET_POWER_STATE;
  // buffer and request size
  request[ 3 ] = 8;
  request[ 4 ] = 8;
  // device id
  request[ 5 ] = MAILBOX_POWER_STATE_DEVICE_SD_CARD;
  // set power off
  request[ 6 ] = MAILBOX_SET_POWER_STATE_WAIT;
  // end tag
  request[ 7 ] = 0;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    free( request );
    return EMMC_RESPONSE_ERROR_IO;
  }
  // handle invalid device id returned
  if ( 0 != request[ 5 ] ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid device id returned\r\n" )
    #endif
    // free request
    free( request );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }
  // check for powered off correctly
  if ( 0 != request[ 6 ] ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Device not powered of successfully: %#"PRIx32"\r\n",
        request[ 6 ]
      )
    #endif
    // free request
    free( request );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }
  // free request
  free( request );
  // return ok
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_controller_startup(void)
 * @brief Startup emmc controller by usind mailbox interface
 *
 * @return
 */
emmc_response_t emmc_controller_startup( void ) {
  // allocate buffer
  size_t request_size;
  int32_t* request = prepare_mailbox( 8, &request_size );
  if ( ! request ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // build request
  // buffer size
  request[ 0 ] = ( int32_t )request_size;
  // perform request
  request[ 1 ] = 0;
  // power state tag
  request[ 2 ] = MAILBOX_SET_POWER_STATE;
  // buffer and request size
  request[ 3 ] = 8;
  request[ 4 ] = 8;
  // device id
  request[ 5 ] = MAILBOX_POWER_STATE_DEVICE_SD_CARD;
  // set power on and wait until it's stable
  request[ 6 ] = MAILBOX_SET_POWER_STATE_ON | MAILBOX_SET_POWER_STATE_WAIT;
  // end tag
  request[ 7 ] = 0;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    free( request );
    return EMMC_RESPONSE_ERROR_IO;
  }
  // handle invalid device id returned
  if ( 0 != request[ 5 ] ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid device id returned\r\n" )
    #endif
    // free
    free( request );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }
  // check for powered on correctly
  if ( ! ( request[ 6 ] & MAILBOX_SET_POWER_STATE_ON ) ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Device not powered of successfully: %#"PRIx32"\r\n",
        request[ 6 ]
      )
    #endif
    // free
    free( request );
    // return error
    return EMMC_RESPONSE_ERROR_IO;
  }
  // free
  free( request );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_controller_restart(void)
 * @brief Restart controller with shutdown, sleep and startup via mailbox
 *
 * @return
 */
emmc_response_t emmc_controller_restart( void ) {
  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Shutdown emmc controller\r\n" )
  #endif
  // shutdown controller
  emmc_response_t response = emmc_controller_shutdown();
  if ( EMMC_RESPONSE_OK != response ) {
    // restarting controller
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to shutdown emmc controller\r\n" )
    #endif
    // return error
    return response;
  }

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Sleeping 5000 microseconds\r\n" )
  #endif
  // wait some time before starting it again
  usleep( 5000 );

  // debug output
  #if defined( EMMC_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Startup emmc controller again\r\n" )
  #endif
  // shutdown controller
  response = emmc_controller_startup();
  if ( EMMC_RESPONSE_OK != response ) {
    // restarting controller
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to startup emmc controller\r\n" )
    #endif
    // return error
    return response;
  }

  // return success
  return EMMC_RESPONSE_OK;
}
