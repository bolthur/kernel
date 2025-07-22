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
 * @brief file descriptor for iomem operations
 */
int fd_iomem = -1;

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
    // free sequence
    free( sequence );
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
    // free sequence
    free( sequence );
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
    // free sequence
    free( sequence );
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
 * @fn response_t dwhci_reset_device(void)
 * @brief Reset dwhci device
 * @return
 */
response_t dwhci_reset_device( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Reset device\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 5, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // loop while ahb idle
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 0 ].loop_and = ( uint32_t )HCD_DWHCI_CORE_RESET_AHB_IDLE;
  sequence[ 0 ].loop_max_iteration = 10;
  sequence[ 0 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 0 ].sleep = 10;
  // read reset value
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  // core soft reset
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 2 ].value = HCD_DWHCI_CORE_RESET_SOFT_RESET;
  // wait until it's gone
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 3 ].loop_and = HCD_DWHCI_CORE_RESET_SOFT_RESET;
  sequence[ 3 ].loop_max_iteration = 10;
  sequence[ 3 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 3 ].sleep = 10;
  // delay 100 ms
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 4 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 4 ].sleep = 100;
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
      STARTUP_PRINT( "Reset sequence failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // check for first timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 0 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Wait for idle timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return timeout
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // check for second timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 3 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Wait for reset timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return timeout
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence again
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_enable_common_interrupts(void)
 * @brief Enable common interrupts
 * @return
 */
response_t dwhci_enable_common_interrupts( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Enable common interrupts\r\n" )
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
  // read interrupt stat
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_INT_STAT;
  // enable all interrupts
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_INT_STAT;
  sequence[ 1 ].value = ( uint32_t )-1;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence failed\r\n" ) /// FIXME: PROVIDE SOME MEANINGFUL OUTPUT
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_read_core_cfg2(uint32_t*)
 * @brief Wrapper to read cfg2
 * @param cfg2
 * @return
 */
response_t dwhci_read_core_cfg2( uint32_t* cfg2 ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Read cfg2\r\n" )
  #endif
  // validate parameter
  if ( ! cfg2 ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "No pointer passed for result\r\n" )
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_HW_CFG2;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of hw cfg2 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // write back result
  *cfg2 = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_read_core_cfg(uint32_t*)
 * @brief Wrapper to read cfg
 * @param cfg
 * @return
 */
response_t dwhci_read_core_cfg( uint32_t* cfg ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Read usb cfg\r\n" )
  #endif
  // validate parameter
  if ( ! cfg ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "No pointer passed for result\r\n" )
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of hw cfg2 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // write back result
  *cfg = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_init_core(void)
 * @brief Init dwhci core
 * @return
 */
response_t dwhci_init_core( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Init core\r\n" )
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
  // read core usb config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_ULPI_EXT_VBUS_DRV;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_TERM_SEL_DL_PULSE;
  // perform request
  int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence failed\r\n" ) /// FIXME: PROVIDE SOME MEANINGFUL OUTPUT
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );

  // reset dwhci device
  response_t result = dwhci_reset_device();
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Reset device failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // allocate sequence
  sequence = util_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read core usb config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_ULPI_UTMI_SEL;
  // write prevoius read with logical and
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  // select utmi+ and utmi width of 8
  sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_PHYIF;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "select utmi+ and utmi width of 8 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );

  uint32_t cfg2;
  result = dwhci_read_core_cfg2( &cfg2 );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of core cfg2 failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // handle wrong architecture
  if ( HCD_DWHCI_CORE_HW_CFG2_ARCHITECTURE( cfg2 ) != 2 ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unsupported architecture\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }

  uint32_t cfg;
  result = dwhci_read_core_cfg( &cfg );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of core cfg failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // set configuration depending on cfg2
  if (
    HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
    && HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE( cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
  ) {
    cfg |=
      HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS
      | HCD_DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M;
  } else {
    cfg &= ( uint32_t )(
      ~HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS
      & ~HCD_DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M
    );
  }

  // write back cfg
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // write back cfg2
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 0 ].value = cfg;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "write back of core usb cfg failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }

  // read cfg2 again
  result = dwhci_read_core_cfg2( &cfg2 );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of core cfg2 failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // validate number of channels
  const uint32_t num_channel = HCD_DWHCI_CORE_HW_CFG2_NUM_HOST_CHANNELS( cfg2 );
  if ( ! ( 4 <= num_channel && num_channel <= PERIPHERAL_DWHCI_MAX_CHANNELS ) ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Invalid number of channels\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }

  // read ahb cfg
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "read of usb core failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // cache it in a variable
  uint32_t ahb_cfg = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // manipulate config
  /// FIXME: ENABLE DMA AND WORK WITH INTERRUPTS
  //ahb_cfg |= HCD_DWHCI_CORE_AHB_CFG_GLOBAL_DMA_ENABLE;
  ahb_cfg |= HCD_DWHCI_CORE_AHB_CFG_GLOBAL_WAIT_AXI_WRITES;
  ahb_cfg &= ( uint32_t )~HCD_DWHCI_CORE_AHB_CFG_GLOBAL_MAX_AXI_BURST_MASK;
  // allocate sequence to write it back
  sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 0 ].value = ahb_cfg;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "write back of ahb config\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );

  // hnp and srp are not used
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_HNP_CAPABLE;
  // write previous read with and
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_SRP_CAPABLE;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "read of usb core failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence again
  free( sequence );

  // enable common interrupts
  result = dwhci_enable_common_interrupts();
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "enable common interrupts failed: %s\r\n",
        response_error( result ) )
    #endif
    // return error
    return result;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_read_host_cfg(uint32_t*)
 * @brief Wrapper to read cfg
 * @param cfg
 * @return
 */
response_t dwhci_read_host_cfg( uint32_t* cfg ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Read usb host cfg\r\n" )
  #endif
  // validate parameter
  if ( ! cfg ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "No pointer passed for result\r\n" )
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of hw cfg2 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // write back result
  *cfg = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_core_flush_tx_fifo(const uint32_t)
 * @brief Wrapper to flush tx fifo
 * @param num_fifo num fifo
 * @return
 *
 * @todo check whether loop is correct
 */
response_t dwhci_core_flush_tx_fifo( const uint32_t num_fifo ) {
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Flushing tx fifo %#"PRIx32"\r\n", num_fifo )
  #endif

  // set initial reset fifo flush
  const uint32_t reset = (
    HCD_DWHCI_CORE_RESET_TX_FIFO_FLUSH
    & ( uint32_t )~HCD_DWHCI_CORE_RESET_TX_FIFO_NUM_MASK
  ) | ( num_fifo << HCD_DWHCI_CORE_RESET_TX_FIFO_NUM_SHIFT );

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
  // write reset config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 0 ].value = reset;
  // loop while it's there
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 1 ].loop_and = ( uint32_t )HCD_DWHCI_CORE_RESET_TX_FIFO_FLUSH;
  sequence[ 1 ].loop_max_iteration = 10;
  sequence[ 1 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 1 ].sleep = 1;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Phy power reset failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // check for timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 1 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Flushing tx fifo timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence again
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_core_flush_rx_fifo(void)
 * @brief Wrapper to flush rx fifo
 * @return
 */
response_t dwhci_core_flush_rx_fifo( void ) {
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Flushing rx fifo\r\n" )
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
  // write reset config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 0 ].value = HCD_DWHCI_CORE_RESET_RX_FIFO_FLUSH;
  // loop while it's there
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 1 ].loop_and = ( uint32_t )HCD_DWHCI_CORE_RESET_RX_FIFO_FLUSH;
  sequence[ 1 ].loop_max_iteration = 10;
  sequence[ 1 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 1 ].sleep = 10;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Phy power reset failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // check for timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 1 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Flushing tx fifo timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence again
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_write_host_port(uint32_t)
 * @brief Helper to write back host port value
 * @param port
 * @return
 */
response_t dwhci_write_host_port( uint32_t port ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Writing host port value %#"PRIx32"\r\n", port )
  #endif
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 1 ].value = port;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of hw cfg2 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;

}

/**
 * @fn response_t dwhci_read_host_cfg(uint32_t*)
 * @brief Wrapper to read cfg
 * @param port
 * @return
 */
response_t dwhci_read_host_port( uint32_t* port ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Read host port\r\n" )
  #endif
  // validate parameter
  if ( ! port ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "No pointer passed for result\r\n" )
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  // perform request
  const int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of hw cfg2 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // write back result
  *port = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_enable_host_interrupts(void);
 * @brief Wrapper to enable host interrupts
 * @return
 */
response_t dwhci_enable_host_interrupts( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Enable host interrupts\r\n" )
  #endif

  // reset core interrupts
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 0 ].value = 0;
  // perform request
  int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of hw cfg2 failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );

  // enable common interrupts again
  const response_t result = dwhci_enable_common_interrupts();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Enable host interrupts failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // read core interrupts again and enable host interrupts
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  // write with or previous read
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 1 ].value = HCD_DWHCI_CORE_INT_MASK_HC_INTR;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Enable of host interrupts failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );

  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_init_host(void)
 * @brief Init dwhci host
 * @return
 */
response_t dwhci_init_host( void ) {
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Initialize host\r\n" )
  #endif
  // set phy power
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_USB_POWER_OFFSET;
  sequence[ 0 ].value = 0;
  // perform request
  int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Phy power reset failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence again
  free( sequence );

  // read out host config
  uint32_t host_cfg;
  response_t result = dwhci_read_host_cfg( &host_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of host cfg failed: %s\r\n", response_error( result ) )
    #endif
    // return error
    return result;
  }
  host_cfg &= ( uint32_t )~HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_MASK;

  // read core cfg
  uint32_t core_cfg2;
  result = dwhci_read_core_cfg2( &core_cfg2 );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of core cfg failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // read core cfg
  uint32_t core_cfg;
  result = dwhci_read_core_cfg( &core_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of core cfg failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // adjust host config
  if (
    HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( core_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
    && HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE( core_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
    && HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS == core_cfg
  ) {
    host_cfg |= HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_48_MHZ;
  } else {
    host_cfg |= HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_30_60_MHZ;
  }

  // write back host config
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 0 ].value = host_cfg;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Phy power reset failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence again
  free( sequence );

  // write rx fifo size, non-periodic tx fifo size and host periodic fifo size
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // write rx fifo size
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_RX_FIFO_SIZ;
  sequence[ 0 ].value = HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE;
  // write non-periodic tx fifo size
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_NPER_FIFO_SIZ;
  sequence[ 1 ].value = HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE |
    ( HCD_DWHCI_CFG_HOST_NPER_TX_FIFO_SIZE << 16 );
  // write host periodic fifo size
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_HOST_PER_TX_FIFO_SZ;
  sequence[ 2 ].value = ( HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE + HCD_DWHCI_CFG_HOST_NPER_TX_FIFO_SIZE )
    | HCD_DWHCI_CFG_HOST_PER_TX_FIFO_SIZE << 16;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Phy power reset failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence again
  free( sequence );

  // flush rx fifo
  result = dwhci_core_flush_tx_fifo( 0x10 );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Flush tx fifo failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // flush tx fifo
  result = dwhci_core_flush_rx_fifo();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Flush rx fifo failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // read port
  uint32_t port;
  result = dwhci_read_host_port( &port );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read host port failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }
  // mask port result
  port &= ( uint32_t )~HCD_DWHCI_HOST_PORT_DEFAULT_MASK;
  // set power if not set
  if ( ! ( port & HCD_DWHCI_HOST_PORT_POWER ) ) {
    // set flag
    port |= HCD_DWHCI_HOST_PORT_POWER;
    // write port back
    // allocate sequence
    sequence = util_prepare_mmio_sequence( 1, &sequence_size );
    if ( ! sequence ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_MEMORY;
    }
    // read config
    sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
    sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
    sequence[ 0 ].value = port;
    // perform request
    ioctl_result = ioctl(
      fd_iomem,
      IOCTL_BUILD_REQUEST(
        IOMEM_RPC_MMIO_PERFORM,
        sequence_size,
        IOCTL_RDWR
      ),
      sequence
    );
    // handle ioctl error
    if ( -1 == ioctl_result ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Write back of host port failed\r\n" )
      #endif
      // free sequence
      free( sequence );
      // return error
      return HCD_RESPONSE_ERROR_IO;
    }
    // free sequence again
    free( sequence );
  }

  // enable host interrupts
  result = dwhci_enable_host_interrupts();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Enable host interrupt failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }

  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_enable_root_port(void)
 * @brief Wrapper to enable root port
 * @return
 */
response_t dwhci_enable_root_port( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Enable root port\r\n" )
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
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 0 ].loop_and = ( uint32_t )HCD_DWHCI_HOST_PORT_CONNECT;
  sequence[ 0 ].loop_max_iteration = 10;
  sequence[ 0 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 0 ].sleep = 10;
  // wait a bit
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 1 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 1 ].sleep = 100;
  // perform request
  int ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Wait for host port command sequence failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // check for valid timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 0 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Wait for host port connect timed out, not really an error\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return success
    return HCD_RESPONSE_OK;
  }
  // free sequence again
  free( sequence );

  // read host port
  uint32_t host_port;
  response_t result = dwhci_read_host_port( &host_port );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read host port failed: %s\r\n", response_error( result ) )
    #endif
    // return result
    return result;
  }
  // mask host port and set reset
  host_port &= ( uint32_t )~HCD_DWHCI_HOST_PORT_DEFAULT_MASK;
  host_port |= HCD_DWHCI_HOST_PORT_RESET;
  // write back value with delay and another write of host port
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 5, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 0 ].value = host_port;
  // wait a bit
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 1 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 1 ].sleep = 50;
  // read host port
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 2 ].value = ( uint32_t )~HCD_DWHCI_HOST_PORT_DEFAULT_MASK;
  // write back with and
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 3 ].value = ( uint32_t )~HCD_DWHCI_HOST_PORT_RESET;
  // wait a bit
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 4 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 4 ].sleep = 20;
  // perform request
  ioctl_result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Write back of host port failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
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

  // enable root port
  result = dwhci_enable_root_port();
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Enable root port failed: %s\r\n", response_error( result ) )
    #endif
    // close fd iomem
    close( fd_iomem );
    // return result
    return result;
  }

  // return success
  return HCD_RESPONSE_OK;
}
