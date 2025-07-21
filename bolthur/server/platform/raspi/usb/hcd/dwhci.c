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
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include <sys/_default_fcntl.h>
// local includes
#include "dwhci.h"
#include "util.h"
#include "response.h"
// driver includes
#include "../../libhcd.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../libmailbox.h"

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
      STARTUP_PRINT( "Querying vendor information failed\r\n" )
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
 * @fn response_t dwhci_power_on(void)
 * @brief Method to power on usb device
 * @return power on result
 */
response_t dwhci_power_on( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Powering on usb device\r\n" )
  #endif
  // allocate buffer
  size_t request_size;
  int32_t* request = util_prepare_mailbox( 8, &request_size );
  if ( ! request ) {
    return HCD_RESPONSE_ERROR_MEMORY;
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
  request[ 5 ] = MAILBOX_POWER_STATE_DEVICE_USB_HCD;
  // set power on and wait until it's stable
  request[ 6 ] = MAILBOX_SET_POWER_STATE_ON | MAILBOX_SET_POWER_STATE_WAIT;
  // end tag
  request[ 7 ] = 0;
  // perform request
  const int result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  // handle ioctl error
  if ( -1 == result ) {
    // free request
    free( request );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // handle not successful
  if ( MAILBOX_REQUEST_SUCCESSFUL != ( uint32_t )request[ 1 ] ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Request not successful: %#"PRIx32"\r\n",
        ( uint32_t )request[ 1 ] )
    #endif
    // free request
    free( request );
    // return error
    return HCD_RESPONSE_ERROR_MAILBOX;
  }
  // handle invalid device id returned
  if ( MAILBOX_POWER_STATE_DEVICE_USB_HCD != request[ 5 ] ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT(
        "Invalid device id returned, expected %#x but received %#"PRIX32"\r\n",
        MAILBOX_POWER_STATE_DEVICE_USB_HCD, request[ 5 ] )
    #endif
    // free
    free( request );
    // return error
    return HCD_RESPONSE_ERROR_MAILBOX;
  }
  // check for powered on correctly
  if ( ( request[ 6 ] & 0x3 ) != MAILBOX_SET_POWER_STATE_ON ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT(
        "Device not powered on successfully: %#"PRIx32"\r\n",
        request[ 6 ] & 0x3 )
    #endif
    // free
    free( request );
    // return error
    return HCD_RESPONSE_ERROR_MAILBOX;
  }
  // free
  free( request );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_enable_global_interrupts( void );
 * @brief Wrapper to enable all interrupts
 * @return
 */
response_t dwhci_enable_global_interrupts( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Enabling all usb interrupts\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read ahb config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  // mask all interrupts
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = HCD_DWHCI_CORE_AHB_CFG_GLOBAL_INTERRUPT_MASK;
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
      STARTUP_PRINT( "Enable all interrupts failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @brief Method to disable global interrupts
 * @return disable interrupt response
 */
response_t dwhci_disable_global_interrupts( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Disabling all usb interrupts\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read ahb config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  // mask all interrupts
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_CORE_AHB_CFG_GLOBAL_INTERRUPT_MASK;
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
      STARTUP_PRINT( "Disable all interrupts failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_register_interrupt(void)
 * @brief Method to register interrupt
 * @return register response
 */
response_t dwhci_register_interrupt( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Try to acquire interrupt %d\r\n", ARM_IRQ_USB )
  #endif
  // try to acquire usb interrupt
  _syscall_interrupt_acquire( ARM_IRQ_USB );
  // handle error
  if ( errno ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to acquire interrupt %d: %s\r\n", ARM_IRQ_USB,
        strerror( errno ) )
    #endif
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_init_core(void)
 * @brief Init dwhci core
 * @return
 */
response_t dwhci_init_core( void ) {
  return HCD_RESPONSE_ERROR_NOT_IMPLEMENTED;
}

/**
 * @fn response_t dwhci_init_host(void)
 * @brief Init dwhci host
 * @return
 */
response_t dwhci_init_host( void ) {
  return HCD_RESPONSE_ERROR_NOT_IMPLEMENTED;
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
    // close fd_iomem again
    close( fd_iomem );
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
      vendor >> 0 & 0xf )
  #endif
  // check fetched vendor
  if ( ( vendor & 0xfffff000 ) != 0x4F542000 ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "HCD: Driver incompatible\r\n" )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return error
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }

  // power on usb device
  result = dwhci_power_on();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to power on usb device: %s\r\n",
        response_error( result ) )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return result
    return result;
  }

  // disable global interrupts
  result = dwhci_disable_global_interrupts();
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to disable global interrupt: %s\r\n",
        response_error( result ) )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return result
    return result;
  }

  // register interrupt handler
  result = dwhci_register_interrupt();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register interrupt: %s\r\n",
        response_error( result ) )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return result
    return result;
  }

  // init core
  result = dwhci_init_core();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to initialize core: %s\r\n",
        response_error( result ) )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return result
    return result;
  }

  // enable global interrupts
  result = dwhci_enable_global_interrupts();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to enable global interrupt: %s\r\n",
        response_error( result ) )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return result
    return result;
  }

  // init core
  result = dwhci_init_host();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to initialize host: %s\r\n",
        response_error( result ) )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return result
    return result;
  }

  // return success
  return HCD_RESPONSE_OK;
}
