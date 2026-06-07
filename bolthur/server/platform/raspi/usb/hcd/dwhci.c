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
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include <sys/_default_fcntl.h>
// local includes
#include "dwhci.h"
#include "response.h"
#include "rpc.h"
// driver includes
#include <sys/mman.h>
// shared includes
#include "../../libhcd.h"
// library includes
#include "../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../library/platform/raspi/iomem/libperipheral.h"
#include "../../../../../library/platform/raspi/iomem/libmailbox.h"
#include "../../../../../library/platform/raspi/iomem/sequence.h"
#include "../../../../../library/platform/raspi/iomem/mailbox.h"

/**
 * @brief file descriptor for iomem operations
 */
int fd_iomem = -1;

/**
 * @brief DWHCI configuration object
 */
dwhci_configuration_t configuration;

/**
 * @fn response_t dwhci_read_port(uint32_t, uint32_t*)
 * @brief Helper to read a port
 * @param port port to read
 * @param value value output variable
 * @return
 *
 * @deprecated
 * @todo make obsolete
 */
response_t dwhci_read_port( const uint32_t port, uint32_t* value ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Query port information from %#"PRIx32"\r\n", port )
  #endif
  // validate parameter
  if ( ! value ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid parameters passed!\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // overwrite register to read
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = port;
  // perform request
  const int result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Read port faild: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // push read value into destination
  *value = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_write_port(uint32_t, uint32_t)
 * @brief Helper to write to a port
 * @param port port to write
 * @param value value to write
 * @return
 *
 * @deprecated
 * @todo make obsolete
 */
response_t dwhci_write_port( const uint32_t port, const uint32_t value ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "write value %#"PRIx32" to port %#"PRIx32"\r\n", value, port )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // overwrite register to read
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = port;
  sequence[ 0 ].value = value;
  // perform request
  const int result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Writing to port failed: %s\r\n", strerror( e ) )
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
 * @fn response_t dwhci_transmit_channel(uint8_t, void*)
 * @brief Transmit channel operation
 * @param channel channel to transmit
 * @param buffer buffer
 * @return
 */
response_t dwhci_transmit_channel( const uint8_t channel, void* buffer ) {
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Transmit channel!\r\n" )
  #endif
  // translate buffer to physical bus address
  const uintptr_t phys = _syscall_memory_translate_bus( ( uintptr_t )buffer, 1 );
  if ( errno ) {
    return HCD_RESPONSE_ERROR_IO;
  }
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 5, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }

  // read split control with unset of complete split
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel );
  sequence[ 0 ].value = ( uint32_t )~( HCD_DWHCI_CHAN_SPLIT_CONTROL_COMPLETE_SPLIT( 1 ) );
  // write back previous read to split control
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ;
  sequence[ 1 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel );
  // set dma address
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_HOST_CHAN_DMA_ADDR( channel );
  sequence[ 2 ].value = phys;
  // read characteristic with unset of packets per frame, enable and disable
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 3 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
  sequence[ 3 ].value = ~(
    ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_PACKETS_PER_FRAME( 0xffffffff )
    | ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 )
    | ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 )
  );
  // write characteristic back with 1 packet per frame and enable 1
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 4 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
  sequence[ 4 ].value = ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_PACKETS_PER_FRAME( 1 )
    | ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
  // write to io
  const int result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Transmit channel sequence failed: %s\r\n",
        strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_prepare_channel(uint32_t, uint32_t, uint8_t, uint32_t, dwhci_channel_state_t, libusb_pipe_address_t*)
 * @brief Prepare channel for transfer
 * @param parent_device_number parent device number
 * @param port_number port number
 * @param channel channel to prepare
 * @param buffer_length buffer length
 * @param packet_id packet id
 * @param usb_pipe pipe to use
 * @return
 */
response_t dwhci_prepare_channel(
  const uint32_t parent_device_number,
  const uint32_t port_number,
  const uint8_t channel,
  const uint32_t buffer_length,
  const dwhci_channel_state_t packet_id,
  const libusb_pipe_address_t* usb_pipe
) {
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "%d / %d / %"PRIu8" / %"PRIu8" / %d / %d\r\n",
      usb_pipe->max_size, usb_pipe->speed, usb_pipe->end_point, usb_pipe->device, usb_pipe->type, usb_pipe->direction )
  #endif
  // prepare characteristic
  const uint32_t characteristic = HCD_DWHCI_CHAN_CHARACTER_DEVICE_ADDRESS( usb_pipe->device )
    | HCD_DWHCI_CHAN_CHARACTER_END_POINT_NUMBER( usb_pipe->end_point )
    | HCD_DWHCI_CHAN_CHARACTER_END_POINT_DIRECTION( usb_pipe->direction )
    | HCD_DWHCI_CHAN_CHARACTER_LOW_SPEED( ( usb_pipe->speed == LIBUSB_SPEED_LOW ? 1 : 0 ) )
    | HCD_DWHCI_CHAN_CHARACTER_TYPE( usb_pipe->type )
    | HCD_DWHCI_CHAN_CHARACTER_MAXIMUM_PACKET_SIZE( usb_number_from_packet_size( usb_pipe->max_size ) )
    | HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 )
    | HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 );
  // prepare split control
  uint32_t split_control = 0;
  if ( LIBUSB_SPEED_HIGH != usb_pipe->speed ) {
    split_control = ( uint32_t )HCD_DWHCI_CHAN_SPLIT_CONTROL_SPLIT_ENABLE( 1 )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_HUB_ADDRESS( parent_device_number )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_PORT_ADDRESS( port_number );
  }
  // prepare transfer data
  uint32_t transfer_data =
    HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( buffer_length )
    | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_ID( packet_id );

  uint32_t packet_count = ( buffer_length + 7 ) / 8;
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "characteristic = %#"PRIx32", split_control = %#"PRIx32", transfer_data = %#"PRIx32", packet_count = %#"PRIx32"\r\n",
    characteristic, split_control, transfer_data, packet_count )
  #endif
  if ( LIBUSB_SPEED_LOW != usb_pipe->speed ) {
    packet_count = (
      buffer_length + usb_number_from_packet_size( usb_pipe->max_size ) - 1 ) / usb_number_from_packet_size( usb_pipe->max_size );
  }
  if ( 0 == packet_count ) {
    packet_count = 1;
  }
  transfer_data |= HCD_DWHCI_CHAN_XFER_SIZE_PACKET_COUNT( packet_count );
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "characteristic = %#"PRIx32", split_control = %#"PRIx32", transfer_data = %#"PRIx32", packet_count = %#"PRIx32"\r\n",
      characteristic, split_control, transfer_data, packet_count )
  #endif
  // allocate mmio sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined ( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate sequence size\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read split control with unset of complete split
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
  sequence[ 0 ].value = characteristic;
  // write back previous read to split control
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel );
  sequence[ 1 ].value = split_control;
  // set dma address
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel );
  sequence[ 2 ].value = transfer_data;
  // write to io
  const int result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Prepare channel sequence failed: %s\r\n",
        strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_allocate_channel(uint8_t*)
 * @brief Method to allocate a channel
 * @param channel_out out pointer where channel is stored
 */
response_t dwhci_allocate_channel( uint8_t* channel_out ) {
  // space for channel mask
  uint32_t mask = 1;
  // iterate through channels
  for (uint32_t channel = 0; channel < configuration.channel.count; channel++) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "configuration.channel.allocated = %#"PRIx32", mask = %#"PRIx32"\r\n",
        configuration.channel.allocated, mask )
    #endif
    // handle channel not allocated
    if (!(configuration.channel.allocated & mask)) {
      // mark it as allocated
      configuration.channel.allocated |= mask;
      // push channel to out field
      *channel_out = (uint8_t)channel;
      // return success
      return HCD_RESPONSE_OK;
    }
    // shift mask to right for check of next channel
    mask <<= 1;
  }
  // error output
  #if defined( DWHCI_ERROR_OUTPUT )
    EARLY_STARTUP_PRINT( "No free channel found\r\n" )
  #endif
  // return no channel
  return HCD_RESPONSE_ERROR_NO_CHANNEL;
}

/**
 * @fn response_t dwhci_free_channel(uint8_t)
 * @brief Helper to free allocated channel
 * @param channel channel to free
 * @return
 */
response_t dwhci_free_channel( const uint8_t channel ) {
  // check channel
  if ( channel >= configuration.channel.count ) {
    // error output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Invalid channel number\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // build mask to apply
  const uint32_t mask = 1 << channel;
  // ensure channel is allocated
  if (!(configuration.channel.allocated & mask)) {
    // error output
    #if defined( DWHCI_ERROR_OUTPUT )
        EARLY_STARTUP_PRINT( "Channel not allocated\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_NO_CHANNEL;
  }
  // deallocate channel
  configuration.channel.allocated &= ~mask;
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_queue_add_entry(void*, size_t, dwhci_queue_status_t, channel_queue_entry_t**)
 * @brief Entry to add to queue
 * @param data data for queue
 * @param size data size
 * @param status queue status
 * @param out pointer to pass object out
 * @return
 */
response_t dwhci_queue_add_entry( void* data, const size_t size, const dwhci_queue_status_t status, channel_queue_entry_t** out ) {
  // allocate entry
  channel_queue_entry_t* entry = malloc( sizeof( *entry ) );
  if (!entry) {
    // some debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate entry for queue\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out everything
  memset( entry, 0, sizeof( *entry ) );
  // prepare entry
  entry->data = data;
  entry->data_size = size;
  entry->status = status;
  entry->error = LIBUSB_TRANSFER_ERROR_NO_ERROR;
  // map buffer
  entry->buffer = mmap( NULL, size, PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_BUS | MAP_DEVICE , -1, 0 );
  // handle map failed
  if ( MAP_FAILED == entry->buffer ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate buffer\r\n" )
    #endif
    // free entry again
    free( entry );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out buffer
  memset( entry->buffer, 0, size );
  // insert into queue
  if ( ! configuration.list ) {
    // list is empty, so just set list
    configuration.list = entry;
  } else {
    // start with beginning
    auto current = configuration.list;
    // loop till end
    while ( current->next ) {
      current = current->next;
    }
    // insert element
    current->next = entry;
    entry->prev = current;
  }
  // handle push to out
  if ( out ) {
    *out = entry;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_queue_remove_entry(channel_queue_entry_t*)
 * @brief Remove given entry from queue
 * @param entry entry to remove
 * @return
 */
response_t dwhci_queue_remove_entry( channel_queue_entry_t* entry ) {
  // validate data
  if ( ! entry ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Invalid entry passed for removal\r\n" )
    #endif
    // return einval
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // handle buffer
  if ( entry->buffer ) {
    munmap( entry->buffer, entry->data_size );
  }
  if ( entry->message ) {
    free( entry->message );
  }
  // handle first element
  if ( entry == configuration.list ) {
    // set list to next
    configuration.list = entry->next;
    // handle list valid
    if ( configuration.list ) {
      // set prev of list to null
      configuration.list->prev = nullptr;
    }
  } else {
    // handle next existing
    if ( entry->next ) {
      // adjust prev pointer of next to prev
      entry->next->prev = entry->prev;
    }
    // handle prev existing
    if ( entry->prev ) {
      // adjust next pointer of prev to next
      entry->prev->next = entry->next;
    }
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_queue_get_active_by_channel(uint8_t, channel_queue_entry_t**)
 * @brief Function to get active queue entry by channel
 * @param channel channel to lookup
 * @param entry output entry
 * @return
 */
response_t dwhci_queue_get_active_by_channel(
  const uint8_t channel,
  channel_queue_entry_t** entry
) {
  // start with queue
  channel_queue_entry_t* current = configuration.list;
  // loop while there is something
  while ( current ) {
    // handle channel match and not status pending
    if (
      current->channel == channel
      && current->status != DWHCI_QUEUE_CHANNEL_STATUS_PENDING
      && current->status != DWHCI_QUEUE_POLL_STATUS_PENDING
    ) {
      // set entry and return success
      *entry = current;
      return HCD_RESPONSE_OK;
    }
    // go to next
    current = current->next;
  }
  // error output
  #if defined( DWHCI_ERROR_OUTPUT )
    EARLY_STARTUP_PRINT( "Invalid channel number\r\n" )
  #endif
  // return einval
  return HCD_RESPONSE_ERROR_EINVAL;
}

/**
 * @fn response_t dwhci_next_usb_pid(dwhci_channel_state_t, uint32_t, uint8_t*)
 * @brief Method to get next usb pid
 * @param last_usb_pid last usb pid
 * @param packet_transferred amount of packets transferred
 * @param output output variable
 * @return
 */
response_t dwhci_next_usb_pid( const dwhci_channel_state_t last_usb_pid, const uint32_t packet_transferred, uint8_t* output ) {
  // validate output
  if ( ! output ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Invalid output passed for Next USB PID\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // evaluate next pid
  switch (last_usb_pid) {
    case DWHCI_CHANNEL_STATE_SETUP:
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Setup state, continue with DATA0\r\n" )
      #endif
      *output = DWHCI_CHANNEL_STATE_DATA1;
      break;
    case DWHCI_CHANNEL_STATE_DATA0:
      if (packet_transferred & 1) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "DATA0 with transfer, continue with DATA1\r\n" )
        #endif
        *output = DWHCI_CHANNEL_STATE_DATA1;
      } else {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "DATA0 with no transfer, continue with DATA0\r\n" )
        #endif
        *output = (uint8_t)last_usb_pid;
      }
      break;
    case DWHCI_CHANNEL_STATE_DATA1:
      if (packet_transferred & 1) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "DATA1 with transfer, continue with DATA0\r\n" )
        #endif
        *output = DWHCI_CHANNEL_STATE_DATA0;
      } else {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "DATA1 with no transfer, continue with DATA1\r\n" )
        #endif
        *output = (uint8_t)last_usb_pid;
      }
      break;
    default:
      // debug output
      #if defined( DWHCI_ERROR_OUTPUT )
        EARLY_STARTUP_PRINT( "Invalid channel passed for Next USB PID\r\n" )
      #endif
      // return einval
      return HCD_RESPONSE_ERROR_EINVAL;
  }
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_enable_channel_interrupt(uint8_t)
 * @brief Method to enable channel interrupt
 * @param channel channel to enable interrupt for
 * @return
 */
response_t dwhci_enable_channel_interrupt( const uint8_t channel ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Enable channel interrupt via mmio sequence\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // prepare sequence
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 1 ].value = 1 << channel;
  // execute sequence
  if ( 0 != iomem_execute_sequence( fd_iomem, sequence, sequence_size ) ) {
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to enable channel interrupt\r\n" )
    #endif
    // release sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // release sequence
  iomem_release_mmio_sequence( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_disable_channel_interrupt(uint8_t)
 * @brief Method to enable channel interrupt
 * @param channel channel to enable interrupt for
 * @return
 *
 * @deprecated
 */
response_t dwhci_disable_channel_interrupt( const uint8_t channel ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Disable channel interrupt via mmio sequence\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // prepare sequence
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 1 ].value = ~(1U << channel);
  // execute sequence
  if ( 0 != iomem_execute_sequence( fd_iomem, sequence, sequence_size ) ) {
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to disable channel interrupt\r\n" )
    #endif
    // release sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // release sequence
  iomem_release_mmio_sequence( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_start_channel(channel_queue_entry_t*)
 * @brief Function to start prepared channel
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_start_channel( const channel_queue_entry_t* entry ) {
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Translate buffer to physical\r\n" )
  #endif
  // translate buffer to physical bus address
  const uintptr_t phys = _syscall_memory_translate_bus(
    ( uintptr_t )entry->buffer, entry->data_size );
  if ( errno ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to translate buffer to phys\r\n" )
    #endif
    // return result
    return HCD_RESPONSE_ERROR_IO;
  }
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Starting async channel via mmio sequence\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 7, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // prepare sequence
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( entry->channel );
  sequence[ 0 ].value = ( uint32_t )-1;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_HOST_CHAN_DMA_ADDR( entry->channel );
  sequence[ 1 ].value = phys + entry->buffer_offset;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 2 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( entry->channel );
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 3 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( entry->channel );
  sequence[ 3 ].value = HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE
    | HCD_CHANNEL_INTERRUPT_HALT
    | HCD_CHANNEL_INTERRUPT_ERROR_MASK
    /// FIXME: ONLY FOR SPLIT OR PREIODIC STUFF
    | HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT
    | HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT
    | HCD_CHANNEL_INTERRUPT_NOT_YET;
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 4 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
  sequence[ 4 ].value = ~HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 );
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 5 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
  sequence[ 5 ].value = HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 6 ].offset = ( uint32_t )PERIPHERAL_DWHCI_CORE_INT_STAT;
  // execute sequence
  if ( 0 != iomem_execute_sequence( fd_iomem, sequence, sequence_size ) ) {
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to disable channel interrupt\r\n" )
    #endif
    // release sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // cache status of 6
  const uint32_t interrupt = sequence[ 6 ].value;
  // release sequence
  iomem_release_mmio_sequence( sequence );
  // handle interrupt
  if ( interrupt & HCD_DWHCI_CORE_INT_MASK_HC_INTR ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Interrupt detected\r\n" )
    #endif
    // calling handler manually
    //rpc_interrupt_handle(0, 0, 0, 0);
  }
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "channel = %"PRIu8"\r\n", entry->channel )
    EARLY_STARTUP_PRINT( "Done\r\n" )
  #endif
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_stop_channel(const channel_queue_entry_t*)
 * @brief
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_stop_channel( const channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Stopping channel via mmio sequence\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // prepare sequence
  // reset enable bit with read
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
  sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
  // set disable bit with write
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
  sequence[ 1 ].value = HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 );
  // read all chan int mask
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  // disable channel with write back
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 3 ].value = ~(1U << entry->channel);
  // execute sequence
  if ( 0 != iomem_execute_sequence( fd_iomem, sequence, sequence_size ) ) {
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to disable channel interrupt\r\n" )
    #endif
    // release sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // release sequence
  iomem_release_mmio_sequence( sequence );
  // free channel again
  const response_t result = dwhci_free_channel( entry->channel );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to free channel %"PRIu8"\r\n", entry->channel )
    #endif
    // return result
    return result;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_setup(channel_queue_entry_t*)
 * @brief Method to start entry transfer with setup packet
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_setup( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Starting setup request: %"PRIu32"\r\n",
      entry->buffer_offset )
  #endif
  const hcd_control_message_t* entry_data = entry->data;
  // create temporary pipe
  const libusb_pipe_address_t setup_pipe = {
    .speed = entry_data->pipe_address.speed,
    .device = entry_data->pipe_address.device,
    .end_point = entry_data->pipe_address.end_point,
    .max_size = entry_data->pipe_address.max_size,
    .type = LIBUSB_TRANSFER_CONTROL,
    .direction = LIBUSB_DIRECTION_OUT,
  };
  // push request into data buffer
  memcpy( entry->buffer, &entry_data->request, sizeof( libusb_device_request_t ) );
  // prepare channel
  const response_t result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    sizeof( libusb_device_request_t ) - entry->buffer_offset,
    DWHCI_CHANNEL_STATE_SETUP,
    &setup_pipe );
  // set buffer size
  entry->buffer_size_to_transfer = sizeof( libusb_device_request_t ) - entry->buffer_offset;
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
  // set last pid to set up
  ( ( hcd_control_message_t* )entry->data )->last_usb_pid = DWHCI_CHANNEL_STATE_SETUP;
  // start send setup packet
  return dwhci_channel_send_async_start_channel( entry );
}

/**
 * @fn response_t dwhci_channel_send_async_data(channel_queue_entry_t*)
 * @brief Method to start entry transfer with state data
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_data( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Starting data request\r\n" )
  #endif
  const hcd_control_message_t* entry_data = entry->data;
  // handle no data to transmit or receive
  if (0 == entry_data->buffer_length ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No data to send continue with ack\r\n" )
    #endif
    // switch to next state
    entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
    // continue directly
    return dwhci_channel_send_async_continue( entry );
  }
  // handle out
  if ( LIBUSB_DIRECTION_OUT == entry_data->pipe_address.direction ) {
    memcpy( entry->buffer, entry_data->buffer, entry_data->buffer_length );
  }
  // create temporary pipe
  const libusb_pipe_address_t data_pipe = {
    .speed = entry_data->pipe_address.speed,
    .device = entry_data->pipe_address.device,
    .end_point = entry_data->pipe_address.end_point,
    .max_size = entry_data->pipe_address.max_size,
    .type = LIBUSB_TRANSFER_CONTROL,
    .direction = entry_data->pipe_address.direction,
  };
  // get next usb pid
  uint8_t next_usb_pid;
  response_t result = dwhci_next_usb_pid( ( ( hcd_control_message_t* )entry->data )->last_usb_pid, entry->packet_transferred, &next_usb_pid );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to get next usb pid\r\n" )
    #endif
    // return result
    return result;
  }
  // prepare channel
  result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    entry_data->buffer_length - entry->buffer_offset,
    next_usb_pid,
    &data_pipe );
  // set buffer size
  entry->buffer_size_to_transfer = entry_data->buffer_length - entry->buffer_offset;
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
  // set last pid to next pid
  const uint8_t previous_usb_pid = ( ( hcd_control_message_t* )entry->data )->last_usb_pid;
  ( ( hcd_control_message_t* )entry->data )->last_usb_pid = next_usb_pid;
  // start send data packet
  result = dwhci_channel_send_async_start_channel( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to start async channel\r\n" )
    #endif
    // reset pid change
    ( ( hcd_control_message_t* )entry->data )->last_usb_pid = previous_usb_pid;
    // return result
    return result;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_ack(channel_queue_entry_t*)
 * @brief Method to start entry transfer with state ack
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_ack( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Starting ack request\r\n" )
  #endif
  hcd_control_message_t* entry_data = entry->data;
  // populate last transfer
  if ( LIBUSB_DIRECTION_IN == entry_data->pipe_address.direction ) {
    entry_data->last_transfer = entry_data->buffer_length;
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "entry->transferred = %"PRIu32", entry_data->buffer_length = %zu\r\n",
        entry->transferred, entry_data->buffer_length );
    #endif
    if ( entry->transferred <= entry_data->buffer_length ) {
      entry_data->last_transfer -= ( entry_data->buffer_length - entry->transferred );
    }
    // copy back data
    memcpy( entry_data->buffer, entry->buffer, entry_data->last_transfer );
  } else {
    entry_data->last_transfer = entry_data->buffer_length;
  }
  // create temporary pipe
  const libusb_pipe_address_t ack_pipe = {
    .speed = entry_data->pipe_address.speed,
    .device = entry_data->pipe_address.device,
    .end_point = entry_data->pipe_address.end_point,
    .max_size = entry_data->pipe_address.max_size,
    .type = LIBUSB_TRANSFER_CONTROL,
    .direction = entry_data->buffer_length == 0
      || entry_data->pipe_address.direction == LIBUSB_DIRECTION_OUT
        ? LIBUSB_DIRECTION_IN
        : LIBUSB_DIRECTION_OUT,
  };
  // push request into data buffer
  memcpy( entry->buffer, &entry_data->request, sizeof( libusb_device_request_t ) );
  // get next usb pid
  uint8_t next_usb_pid;
  response_t result = dwhci_next_usb_pid( ( ( hcd_control_message_t* )entry->data )->last_usb_pid, entry->packet_transferred, &next_usb_pid );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to get next usb pid\r\n" )
    #endif
    // return result
    return result;
  }
  // prepare channel
  result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    0,
    next_usb_pid,
    &ack_pipe );
  // set buffer size
  entry->buffer_size_to_transfer = 0;
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
  // set last pid to next pid
  const uint8_t previous_usb_pid = ( ( hcd_control_message_t* )entry->data )->last_usb_pid;
  ( ( hcd_control_message_t* )entry->data )->last_usb_pid = next_usb_pid;
  // start send data packet
  result = dwhci_channel_send_async_start_channel( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to start async channel\r\n" )
    #endif
    // reset pid change
    ( ( hcd_control_message_t* )entry->data )->last_usb_pid = previous_usb_pid;
    // return result
    return result;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_done(channel_queue_entry_t*)
 * @brief Method to finish entry transfer after ack
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_done( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handling finished request\r\n" )
  #endif
  // handle transfer size not null
  if ( entry->transferred ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Warning non zero status transfer: %"PRIu32"\r\n", entry->transferred )
    #endif
  }
  // stop transmission
  const response_t result = dwhci_channel_send_async_stop_channel( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to stop channel\r\n")
    #endif
  }
  // finally set no error
  if ( entry->error ) {
    entry->error |= LIBUSB_TRANSFER_ERROR_PROCESSING;
  }
  ( ( hcd_control_message_t* )entry->data )->error = entry->error;
  // allocate response structure
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( hcd_submit_control_message_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate memory for response\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out memory
  memset( response, 0, response_size );
  // detach shared memory
  _syscall_memory_shared_detach( ( ( hcd_submit_control_message_t* )entry->message )->shm_id );
  // populate response
  memcpy( response->container, entry->message, sizeof( hcd_submit_control_message_t ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, entry->response_info );
  // free entry
  free( response );
  // destroy queue entry
  return dwhci_queue_remove_entry( entry );
}

/**
 * @fn response_t dwhci_channel_send_async_continue_pending(channel_queue_entry_t*)
 * @brief Method to start entry transfer with state pending
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_continue_pending( [[maybe_unused]] channel_queue_entry_t* entry ) {
  /// FIXME: TAKE NEXT PENDING ENTRY FROM LIST
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_continue(channel_queue_entry_t*)
 * @brief Method to start entry transfer depending on status
 * @param entry
 * @return
 */
response_t dwhci_channel_send_async_continue( channel_queue_entry_t* entry ) {
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Continuing with channel %"PRIu8"\r\n", entry->channel )
  #endif
  // continue channel depending on status
  switch ( entry->status ) {
    case DWHCI_QUEUE_CHANNEL_STATUS_SETUP:
      return dwhci_channel_send_async_setup( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_DATA:
      return dwhci_channel_send_async_data( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_ACK:
      return dwhci_channel_send_async_ack( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_DONE:
      return dwhci_channel_send_async_done( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_PENDING:
      return dwhci_channel_send_async_continue_pending( entry );
    case DWHCI_QUEUE_POLL_STATUS_DATA:
      return dwhci_channel_poll_async_data( entry );
    case DWHCI_QUEUE_POLL_STATUS_ACK:
      return dwhci_channel_poll_async_ack( entry );
    case DWHCI_QUEUE_POLL_STATUS_DONE:
      return dwhci_channel_poll_async_done( entry );
    case DWHCI_QUEUE_POLL_STATUS_PENDING:
      return dwhci_channel_send_async_continue_pending( entry );
    default:
      return HCD_RESPONSE_ERROR_UNKNOWN;
  }
}

/**
 * @fn response_t dwhci_channel_send_async(hcd_control_message_t*, size_t, hcd_submit_control_message_t*, size_t);
 * @brief Wrapper to perform async channel send
 * @param data data to send
 * @param data_size data size
 * @param message original message
 * @param response_info where to respond result to
 * @return
 */
response_t dwhci_channel_send_async( hcd_control_message_t* data, size_t data_size, hcd_submit_control_message_t* message, const size_t response_info ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT("Channel send async\r\n")
  #endif
  // push data with channel to queue
  channel_queue_entry_t* entry = nullptr;
  response_t result = dwhci_queue_add_entry( data, data_size, DWHCI_QUEUE_CHANNEL_STATUS_PENDING, &entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add request to queue\r\n" )
    #endif
    // return result
    return result;
  }
  // duplicate message
  hcd_submit_control_message_t* dup_message = malloc( sizeof( *dup_message ) );
  if ( ! dup_message ) {
    // clear entry again
    dwhci_queue_remove_entry( entry );
    // return no memory
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  memcpy( dup_message, message, sizeof( *dup_message ) );
  // populate response info
  entry->response_info = response_info;
  entry->message = dup_message;
  // try to allocate a channel
  uint8_t channel = 0;
  result = dwhci_allocate_channel( &channel );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate a channel, entry is queued\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_OK;
  }
  // set allocated channel
  entry->channel = channel;
  entry->status = DWHCI_QUEUE_CHANNEL_STATUS_SETUP;
  // enable channel interrupt
  result = dwhci_enable_channel_interrupt( channel );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to enable channel interrupt\r\n" )
    #endif
    // free channel again
    dwhci_free_channel( channel );
    // remove from queue again
    dwhci_queue_remove_entry( entry );
    // return result
    return result;
  }
  // continue async
  return dwhci_channel_send_async_continue( entry );
}

/**
 * @fn response_t dwhci_channel_send_async_data(channel_queue_entry_t*)
 * @brief Method to start entry transfer with state data
 * @param entry
 * @return
 */
response_t dwhci_channel_poll_async_data( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Starting data request\r\n" )
  #endif
  const hcd_interrupt_poll_t* entry_data = entry->data;
  // handle no data to transmit or receive
  if (0 == entry_data->buffer_length ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No data to send continue with ack\r\n" )
    #endif
    // switch to next state
    entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
    // continue directly
    return dwhci_channel_send_async_continue( entry );
  }
  // handle out
  if ( LIBUSB_DIRECTION_OUT == entry_data->pipe_address.direction ) {
    memcpy( entry->buffer, entry_data->buffer, entry_data->buffer_length );
  }
  // create temporary pipe
  const libusb_pipe_address_t data_pipe = {
    .speed = entry_data->pipe_address.speed,
    .device = entry_data->pipe_address.device,
    .end_point = entry_data->pipe_address.end_point,
    .max_size = entry_data->pipe_address.max_size,
    .type = entry_data->pipe_address.type,
    .direction = entry_data->pipe_address.direction,
  };
  // get next usb pid
  uint8_t next_usb_pid;
  response_t result = dwhci_next_usb_pid( ( ( hcd_interrupt_poll_t* )entry->data )->last_usb_pid,
    ( ( hcd_interrupt_poll_t* )entry->data )->previous_transferred_packet, &next_usb_pid );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to get next usb pid\r\n" )
    #endif
    // return result
    return result;
  }
  // prepare channel
  result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    entry_data->buffer_length - entry->buffer_offset,
    next_usb_pid,
    &data_pipe );
  // set buffer size
  entry->buffer_size_to_transfer = entry_data->buffer_length - entry->buffer_offset;
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
  // start send data packet
  return dwhci_channel_send_async_start_channel( entry );
}

/**
 * @fn response_t dwhci_channel_send_async_ack(channel_queue_entry_t*)
 * @brief Method to start entry transfer with state ack
 * @param entry
 * @return
 */
response_t dwhci_channel_poll_async_ack( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Starting ack request\r\n" )
  #endif
  hcd_interrupt_poll_t* entry_data = entry->data;
  // populate last transfer
  if ( LIBUSB_DIRECTION_IN == entry_data->pipe_address.direction ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "DIRECTION IN POLL ASYNC ACK\r\n" )
    #endif
    // set last transfer to 0
    entry_data->last_transfer = 0;
    // set last transfer to buffer length if not a nack
    if ( ! ( entry->error & LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE ) ) {
      // set last transfer
      entry_data->last_transfer = entry_data->buffer_length;
      // copy back data
      memcpy( entry_data->buffer, entry->buffer, entry_data->last_transfer );
    }
  } else {
    entry_data->last_transfer = entry_data->buffer_length;
  }
  entry_data->previous_transferred_packet = entry->packet_transferred;
  entry->status = DWHCI_QUEUE_POLL_STATUS_DONE;
  return dwhci_channel_send_async_continue( entry );
}

/**
 * @fn response_t dwhci_channel_send_async_done(channel_queue_entry_t*)
 * @brief Method to finish entry transfer after ack
 * @param entry
 * @return
 */
response_t dwhci_channel_poll_async_done( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handling finished request\r\n" )
  #endif
  // stop transmission
  const response_t result = dwhci_channel_send_async_stop_channel( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to stop channel\r\n")
    #endif
  }
  // finally set no error
  if ( entry->error ) {
    entry->error |= LIBUSB_TRANSFER_ERROR_PROCESSING;
  }
  ( ( hcd_interrupt_poll_t* )entry->data )->error = entry->error;
  // allocate response structure
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( hcd_submit_interrupt_poll_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate memory for response\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out memory
  memset( response, 0, response_size );
  // detach shared memory
  _syscall_memory_shared_detach( ( ( hcd_submit_interrupt_poll_t* )entry->message )->shm_id );
  // populate response
  memcpy( response->container, entry->message, sizeof( hcd_submit_interrupt_poll_t ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL, entry->response_info );
  // free entry
  free( response );
  // destroy queue entry
  return dwhci_queue_remove_entry( entry );
}

/**
 * @fn response_t dwhci_channel_poll_async(hcd_interrupt_poll_t*, size_t, hcd_submit_interrupt_poll_t*, size_t);
 * @brief Wrapper to perform async channel polling
 * @param data data to send
 * @param data_size data size
 * @param message original message
 * @param response_info where to respond result to
 * @return
 */
response_t dwhci_channel_poll_async(
  hcd_interrupt_poll_t* data,
  const size_t data_size,
  hcd_submit_interrupt_poll_t* message,
  const size_t response_info
) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT("Channel send async\r\n")
  #endif
  // push data with channel to queue
  channel_queue_entry_t* entry = nullptr;
  response_t result = dwhci_queue_add_entry( data, data_size, DWHCI_QUEUE_POLL_STATUS_PENDING, &entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to add request to queue\r\n" )
    #endif
    // return result
    return result;
  }
  // duplicate message
  hcd_submit_interrupt_poll_t* dup_message = malloc( sizeof( *dup_message ) );
  if ( ! dup_message ) {
    // clear entry again
    dwhci_queue_remove_entry( entry );
    // return no memory
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  memcpy( dup_message, message, sizeof( *dup_message ) );
  // populate response info
  entry->response_info = response_info;
  entry->message = dup_message;
  // try to allocate a channel
  uint8_t channel = 0;
  result = dwhci_allocate_channel( &channel );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate a channel, entry is queued\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_OK;
  }
  // set allocated channel
  entry->channel = channel;
  entry->status = DWHCI_QUEUE_POLL_STATUS_DATA;
  // enable channel interrupt
  result = dwhci_enable_channel_interrupt( channel );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to enable channel interrupt\r\n" )
    #endif
    // free channel again
    dwhci_free_channel( channel );
    // remove from queue again
    dwhci_queue_remove_entry( entry );
    // return result
    return result;
  }
  // continue async
  return dwhci_channel_send_async_continue( entry );
}

/**
 * @fn response_t dwhci_power_on(void)
 * @brief Method to power on usb device
 * @return power on result
 */
response_t dwhci_power_on( void ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Powering on usb device\r\n" )
  #endif
  // allocate buffer
  size_t request_size;
  int32_t* request = iomem_prepare_mailbox( 8, &request_size );
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
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Request not successful: %#"PRIx32"\r\n",
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
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT(
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
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT(
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
    EARLY_STARTUP_PRINT( "Flushing tx fifo %#"PRIx32"\r\n", num_fifo )
  #endif

  // set initial reset fifo flush
  const uint32_t reset = HCD_DWHCI_CORE_RESET_TX_FIFO_FLUSH
   | ( num_fifo << HCD_DWHCI_CORE_RESET_TX_FIFO_NUM_SHIFT );

  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
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
  const int ioctl_result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Flush tx fifo failed: %s\r\n", strerror( e ) )
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
      EARLY_STARTUP_PRINT( "Flushing tx fifo timed out\r\n" )
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
    EARLY_STARTUP_PRINT( "Flushing rx fifo\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
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
  const int ioctl_result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == ioctl_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "flush rx fifo failed: %s\r\n", strerror( e ) )
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
      EARLY_STARTUP_PRINT( "Flushing tx fifo timed out\r\n" )
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
 * @fn response_t dwhci_init(void)
 * @brief Init dwhci otg
 * @return
 */
response_t dwhci_init( void ) {
  // open file descriptor for mmio actions
  if ( -1 == ( fd_iomem = open( IOMEM_DEVICE_PATH, O_RDWR ) ) ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to open device\r\n" )
    #endif
    // return error response
    return HCD_RESPONSE_ERROR_IO;
  }
  // clear out configuration object
  memset( &configuration, 0, sizeof( configuration ) );
  // query vendor and hardware information
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 7, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // prepare mmio sequence
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_VENDOR_ID;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_USER_ID;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_HW_CFG1;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_CORE_HW_CFG2;
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 4 ].offset = PERIPHERAL_DWHCI_CORE_HW_CFG3;
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 5 ].offset = PERIPHERAL_DWHCI_CORE_HW_CFG4;
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 6 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  // perform request
  int result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Querying vendor information failed: %s\r\n",
        strerror( e ) )
    #endif
    // close file descriptor
    close( fd_iomem );
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // cache everything locally
  const uint32_t vendor = sequence[ 0 ].value;
  [[maybe_unused]] const uint32_t user = sequence[ 1 ].value;
  [[maybe_unused]] const uint32_t hw_cfg1 = sequence[ 2 ].value;
  uint32_t hw_cfg2 = sequence[ 3 ].value;
  [[maybe_unused]] const uint32_t hw_cfg3 = sequence[ 4 ].value;
  [[maybe_unused]] const uint32_t hw_cfg4 = sequence[ 5 ].value;
  uint32_t host_cfg = sequence[ 6 ].value;
  // free sequence
  free( sequence );
  // check fetched vendor
  if ( ( vendor & 0xfffff000 ) != 0x4F542000 ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "HCD: Driver incompatible\r\n" )
    #endif
    // close fd_iomem again
    close( fd_iomem );
    // return error
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT(
      "HCD: Hardware: %c%c%"PRIx32".%"PRIx32"%"PRIx32"%"PRIx32" (BCM%.5"PRIx32")\r\n",
      ( char )( vendor >> 24 & 0xff ),
      ( char )( vendor >> 16 & 0xff ),
      vendor >> 12 & 0xf,
      vendor >> 8 & 0xf,
      vendor >> 4 & 0xf,
      vendor >> 0 & 0xf,
      user >> 12 & 0xffff
    )
    EARLY_STARTUP_PRINT( "Hardware configuration: %#08"PRIx32" %#08"PRIx32" %#08"PRIx32" %#08"PRIx32"\r\n",
      hw_cfg1, hw_cfg2, hw_cfg3, hw_cfg4 )
    EARLY_STARTUP_PRINT( "Host configuration: %#08"PRIx32"\r\n", host_cfg )
  #endif
  // check architecture
  if ( HCD_DWHCI_CORE_HW_CFG2_ARCHITECTURE( hw_cfg2 ) != HCD_DWHCI_CORE_HW_CFG2_ARCHITECTURE_INTERNAL_DMA ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Host architecture is not internal DMA\r\n" )
    #endif
    // close descriptor
    close( fd_iomem );
    // return incompatible
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }
  // check hs phy
  /*if ( HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_NOT_SUPPORTED ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "High speed physical not supported\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return incompatible
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }*/
  // disable interrupts
  sequence = iomem_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 0 ].value = 0;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_CORE_AHB_CFG_GLOBAL_INTERRUPT_MASK;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Disable of interrupts failed: %s\r\n",
        strerror( e ) )
    #endif
    // close file descriptor
    close( fd_iomem );
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // power on usb hub
  response_t dwhci_result = dwhci_power_on();
  // handle error
  if ( HCD_RESPONSE_OK != dwhci_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to power on hub\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return error
    return dwhci_result;
  }
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Disable pulse and vbus and perform initial reset\r\n" )
  #endif
  // allocate sequence
  sequence = iomem_prepare_mmio_sequence( 9, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
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
  // loop while ahb idle
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 2 ].loop_and = ( uint32_t )HCD_DWHCI_CORE_RESET_AHB_IDLE;
  sequence[ 2 ].loop_max_iteration = 10;
  sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 2 ].sleep = 10;
  // read reset value
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  // core soft reset
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 4 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 4 ].value = HCD_DWHCI_CORE_RESET_SOFT_RESET;
  // wait until it's gone
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
  sequence[ 5 ].offset = PERIPHERAL_DWHCI_CORE_RESET;
  sequence[ 5 ].loop_and = HCD_DWHCI_CORE_RESET_SOFT_RESET;
  sequence[ 5 ].loop_max_iteration = 10;
  sequence[ 5 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 5 ].sleep = 10;
  // delay 100 ms
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 6 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 6 ].sleep = 100;
  // select utmi+ and utmi width of 8
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 7 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 7 ].value = ~HCD_DWHCI_CORE_USB_CFG_ULPI_UTMI_SEL;
  sequence[ 8 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 8 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 8 ].value = ~HCD_DWHCI_CORE_USB_CFG_PHYIF;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // check for first timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 2 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for idle timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return timeout
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // check for second timeout
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 5 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Wait for reset timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return timeout
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence
  iomem_release_mmio_sequence( sequence );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Preparing usb configuration\r\n" )
  #endif
  // query standalone register
  uint32_t usb_cfg;
  dwhci_result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_USB_CFG, &usb_cfg );
  if ( HCD_RESPONSE_OK != dwhci_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Read port failed\r\n" )
    #endif
    return dwhci_result;
  }
  if (
    HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
    && HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
  ) {
    // enable configuration
    usb_cfg |= HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS | HCD_DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M;
  } else {
    // disable configuration
    usb_cfg &= ~HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS;
    usb_cfg &= ~HCD_DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M;
  }
  // write back value
  dwhci_result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_USB_CFG, usb_cfg );
  if ( HCD_RESPONSE_OK != dwhci_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Write port failed\r\n" )
    #endif
    return dwhci_result;
  }

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Preparing dma configuration\r\n" )
  #endif
  // prepare sequence
  sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read core usb config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_OR;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 0 ].value = HCD_DWHCI_CORE_AHB_CFG_GLOBAL_DMA_ENABLE
    | HCD_DWHCI_CORE_AHB_CFG_GLOBAL_WAIT_AXI_WRITES;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = ~HCD_DWHCI_CORE_AHB_CFG_GLOBAL_MAX_AXI_BURST_MASK;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  iomem_release_mmio_sequence( sequence );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Preparing further usb configuration and reset interrupts\r\n" )
  #endif
  // prepare sequence
  sequence = iomem_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // read core usb config
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 0 ].value = ~HCD_DWHCI_CORE_USB_CFG_HNP_CAPABLE;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = ~HCD_DWHCI_CORE_USB_CFG_SRP_CAPABLE;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_INT_STAT;
  sequence[ 2 ].value = -1U;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  iomem_release_mmio_sequence( sequence );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Enabling global interrupts\r\n" )
  #endif
  // enable all interrupts
  sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // enable core interrupts
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_OR;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 0 ].value = HCD_DWHCI_CORE_AHB_CFG_GLOBAL_INTERRUPT_MASK;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  iomem_release_mmio_sequence( sequence );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Restart phy clock\r\n" )
  #endif
  // enable all interrupts
  sequence = iomem_prepare_mmio_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // enable core interrupts
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_USB_POWER_OFFSET;
  sequence[ 0 ].value = 0;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 1 ].value = ~HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_MASK;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_HW_CFG2;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // store values in variables
  host_cfg = sequence[ 1 ].value;
  hw_cfg2 = sequence[ 2 ].value;
  usb_cfg = sequence[ 3 ].value;
  // release sequence
  iomem_release_mmio_sequence( sequence );

  if (
    HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
    && HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
    && usb_cfg & HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS
  ) {
    host_cfg |= HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_48_MHZ;
  } else {
    host_cfg |= HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_30_60_MHZ;
  }

  // enable all interrupts
  sequence = iomem_prepare_mmio_sequence( 4, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // enable core interrupts
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 0 ].value = host_cfg;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_RX_FIFO_SIZ;
  sequence[ 1 ].value = HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_NPER_FIFO_SIZ;
  sequence[ 2 ].value = HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE
    | HCD_DWHCI_CFG_HOST_NPER_TX_FIFO_SIZE << 16;
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_CORE_HOST_PER_TX_FIFO_SZ;
  sequence[ 3 ].value = ( HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE + HCD_DWHCI_CFG_HOST_NPER_TX_FIFO_SIZE )
    | HCD_DWHCI_CFG_HOST_PER_TX_FIFO_SIZE << 16;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // store values in variables
  host_cfg = sequence[ 1 ].value;
  hw_cfg2 = sequence[ 2 ].value;
  usb_cfg = sequence[ 3 ].value;
  // release sequence
  iomem_release_mmio_sequence( sequence );

  dwhci_core_flush_tx_fifo(0x10);
  dwhci_core_flush_rx_fifo();

  // read out host config
  dwhci_result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CFG, &host_cfg );
  if ( HCD_RESPONSE_OK != dwhci_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Read of host cfg failed: %s\r\n", response_error( dwhci_result ) )
    #endif
    // return error
    return dwhci_result;
  }
  // put channels into known states if no dma descriptor is enabled
  if ( ! ( host_cfg & HCD_DWHCI_HOST_CFG_ENABLE_DMA_DESCRIPTOR ) ) {
    // prepare sequence
    sequence = iomem_prepare_mmio_sequence( 2, &sequence_size );
    if ( ! sequence ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_MEMORY;
    }
    // extract channel count
    configuration.channel.count = HCD_DWHCI_CORE_HW_CFG2_NUM_HOST_CHANNELS( hw_cfg2 );
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "configuration.channel.count = %"PRIu32"\r\n", configuration.channel.count )
    #endif
    // loop over channels
    for ( uint32_t channel = 0; channel < configuration.channel.count; ++channel ) {
      sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
      sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
      sequence[ 0 ].value = ( uint32_t )~(
        HCD_DWHCI_CHAN_CHARACTER_END_POINT_DIRECTION( 1 )
        | HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 )
        | HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 )
      );
      sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
      sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
      sequence[ 1 ].value = HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 )
        | HCD_DWHCI_CHAN_CHARACTER_END_POINT_DIRECTION( 1 );
      // perform request
      result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
      // handle ioctl error
      if ( -1 == result ) {
        // debug output
        #if defined( DWHCI_ERROR_OUTPUT )
          const int e = errno;
          EARLY_STARTUP_PRINT( "host config sequence failed: %s\r\n", strerror( e ) )
        #endif
        // free sequence
        free( sequence );
        // return error
        return HCD_RESPONSE_ERROR_IO;
      }
    }
    // free sequence again
    iomem_release_mmio_sequence( sequence );
    // prepare sequence
    sequence = iomem_prepare_mmio_sequence( 3, &sequence_size );
    if ( ! sequence ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_MEMORY;
    }
    // loop through channels again
    for ( uint32_t channel = 0; channel < configuration.channel.count; ++channel ) {
      // read channel
      sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
      sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
      // set enable disable and end point direction
      sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
      sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
      sequence[ 1 ].value = HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 )
        | ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 )
        | HCD_DWHCI_CHAN_CHARACTER_END_POINT_DIRECTION( 1 );
      // wait until enable is gone
      sequence[ 2 ].type = IOMEM_MMIO_ACTION_LOOP_TRUE;
      sequence[ 2 ].offset = PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
      sequence[ 2 ].loop_and = ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
      sequence[ 2 ].loop_max_iteration = 10;
      sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
      sequence[ 2 ].sleep = 10;
      // perform request
      result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
      // handle ioctl error
      if ( -1 == result ) {
        // debug output
        #if defined( DWHCI_ERROR_OUTPUT )
          const int e = errno;
          EARLY_STARTUP_PRINT( "host config sequence failed: %s\r\n", strerror( e ) )
        #endif
        // free sequence
        iomem_release_mmio_sequence( sequence );
        // return error
        return HCD_RESPONSE_ERROR_IO;
      }
      // check for timeout
      if ( sequence[ 2 ].abort_type == IOMEM_MMIO_ABORT_TYPE_TIMEOUT ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to clear halt on channel %"PRIu32"\r\n",
            channel )
        #endif
      }
    }
    // free sequence
    iomem_release_mmio_sequence( sequence );
  }

  uint32_t host_port;
  dwhci_result = dwhci_read_port(PERIPHERAL_DWHCI_HOST_PORT, &host_port);
  if ( HCD_RESPONSE_OK != dwhci_result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      EARLY_STARTUP_PRINT( "Read port failed\r\n" )
    #endif
    return dwhci_result;
  }
  if ( ! ( host_port & HCD_DWHCI_HOST_PORT_POWER ) ) {
    host_port |= HCD_DWHCI_HOST_PORT_POWER;
    dwhci_result = dwhci_write_port(PERIPHERAL_DWHCI_HOST_PORT, host_port);
    if ( HCD_RESPONSE_OK != dwhci_result ) {
      // debug output
      #if defined( DWHCI_ERROR_OUTPUT )
        EARLY_STARTUP_PRINT( "Read port failed\r\n" )
      #endif
      return dwhci_result;
    }
  }

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Enabling interrupts\r\n" )
  #endif
  // enable all interrupts
  sequence = iomem_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // enable host interrupts
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 0 ].value = 0;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 2 ].value = (HCD_DWHCI_CORE_INT_MASK_HC_INTR/*
    | HCD_DWHCI_CORE_INT_MASK_PORT_INTR
    | HCD_DWHCI_CORE_INT_MASK_DISCONNECT
    | HCD_DWHCI_CORE_INT_MASK_USB_SUSPEND
    | HCD_DHWCI_CORE_INT_MASK_OTG_INTR
    | HCD_DWHCI_CORE_INT_MASK_SOF_INTR
    | HCD_DWHCI_CORE_INT_MASK_RX_STS_Q_LVL
    | HCD_DWHCI_CORE_INT_MASK_CON_ID_STS_CHNG
    | HCD_DWHCI_CORE_INT_MASK_SESS_REQ_INTR
    | HCD_DWHCI_CORE_INT_MASK_WKUP_INTR*/);
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Enable of interrupts failed: %s\r\n", strerror( e ) )
    #endif
    // close file descriptor
    close( fd_iomem );
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  iomem_release_mmio_sequence( sequence );

  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Enable root port\r\n" )
  #endif
  // prepare sequence
  sequence = iomem_prepare_mmio_sequence( 8, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // wait until port connect is gone
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_LOOP_FALSE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 0 ].loop_and = ( uint32_t )HCD_DWHCI_HOST_PORT_CONNECT;
  sequence[ 0 ].loop_max_iteration = 10;
  sequence[ 0 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 0 ].sleep = 10;
  // delay 100ms
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 1 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 1 ].sleep = 100;
  // read host port
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 2 ].value = ~HCD_DWHCI_HOST_PORT_DEFAULT_MASK;
  // write back "orred" with reset
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 3 ].value = HCD_DWHCI_HOST_PORT_RESET;
  // delay 50 to 60ms
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 4 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 4 ].sleep = 60;
  // read host port again
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 5 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 5 ].value = ~HCD_DWHCI_HOST_PORT_DEFAULT_MASK;
  // write back "orred" with power
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 6 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 6 ].value = HCD_DWHCI_HOST_PORT_POWER;
  // delay again for 20ms
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 7 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 7 ].sleep = 20;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ERROR_OUTPUT )
      const int e = errno;
      EARLY_STARTUP_PRINT( "host config sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  iomem_release_mmio_sequence( sequence );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Acquiring interrupt %d\r\n", ARM_IRQ_USB )
  #endif
  // register interrupt
  _syscall_interrupt_acquire( ARM_IRQ_USB );

  // return success
  return HCD_RESPONSE_OK;
}
