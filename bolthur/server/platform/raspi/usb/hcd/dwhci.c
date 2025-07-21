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
#include <sys/ioctl.h>
// local includes
#include "dwhci.h"
#include "util.h"
#include "response.h"
// driver includes
#include <sys/_default_fcntl.h>

#include "../../libiomem.h"
#include "../../libperipheral.h"

/**
 * @brief Static file descriptor for iomem operations
 */
static int fd_iomem = -1;

/**
 * @fn uint32_t dwhci_query_vendor(uint32_t*)
 * @brief Helper to query vendor
 * @param destination pointer to write vendor id to
 * @return operation response
 */
response_t dwhci_query_vendor( uint32_t* destination ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Query vendor information\r\n" )
  #endif
  // validate parameter
  if ( ! destination ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid parameters passed!\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // overwrite register to read
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_VENDOR_ID;
  // perform request
  const int result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Mark interrupt as handled sequence failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // push read value into destination
  *destination = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_init(void)
 * @brief Init dwhci
 * @return init response
 */
response_t dwhci_init( void ) {
  // open file descriptor for mmio actions
  if ( -1 == ( fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR ) ) ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to open device\r\n" )
    #endif
    // return error response
    return HCD_RESPONSE_ERROR_IO;
  }
  // query vendor id
  uint32_t vendor;
  response_t result = dwhci_query_vendor( &vendor );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Query vendor information failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT(
      "HCD: Hardware: %c%c%"PRIx32".%"PRIx32"%"PRIx32"%"PRIx32"\r\n",
      ( char )( vendor >> 24 & 0xff ),
      ( char )( vendor >> 16 & 0xff ),
      vendor >> 12 & 0xf,
      vendor >> 8 & 0xf,
      vendor >> 4 & 0xf,
      vendor >> 0 & 0xf
    )
  #endif
  // check fetched vendor
  if ( ( vendor & 0xfffff000 ) != 0x4F542000 ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "HCD: Driver incompatible\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }
  /// FIXME: CONTINUE HCD INIT
  // return success
  return HCD_RESPONSE_OK;
}
