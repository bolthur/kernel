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
#include "constants.h"
// driver includes
#include <sys/mman.h>
// shared includes
#include "../../libhcd.h"
// library includes
#include "mmio.h"
#include "timer.h"
#include "../../../../libusbd.h"
#include "../../../../../library/platform/raspi/iomem/libiomem.h"
#include "../../../../../library/platform/raspi/iomem/libperipheral.h"
#include "../../../../../library/platform/raspi/iomem/libmailbox.h"
#include "../../../../../library/platform/raspi/iomem/sequence.h"
#include "../../../../../library/platform/raspi/iomem/mailbox.h"
#include "../../../../../library/usb/usb.h"

/**
 * @brief file descriptor for iomem operations
 */
int fd_iomem = -1;

/**
 * @brief DWHCI configuration object
 */
dwhci_configuration_t configuration;

/**
 * @fn response_t dwhci_prepare_channel(uint32_t, uint32_t, uint8_t, uint32_t, dwhci_channel_state_t, libusb_pipe_address_t*, uint32_t, bool, channel_queue_entry_t*)
 * @brief Prepare channel for transfer
 * @param parent_device_number parent device number
 * @param port_number port number
 * @param channel channel to prepare
 * @param buffer_length buffer length
 * @param packet_id packet id
 * @param usb_pipe pipe to use
 * @param interval interval for interrupt polling
 * @param channel_prepared flag indicating whether channel is already prepared
 * @param entry queue entry itself
 * @return
 */
response_t dwhci_prepare_channel(
  const uint32_t parent_device_number,
  const uint32_t port_number,
  const uint8_t channel,
  uint32_t buffer_length,
  const dwhci_channel_state_t packet_id,
  const libusb_pipe_address_t* usb_pipe,
  const uint32_t interval,
  const bool channel_prepared,
  channel_queue_entry_t* entry
) {
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "%d / %d / %"PRIu8" / %"PRIu8" / %d / %d\r\n",
      usb_pipe->max_size, usb_pipe->speed, usb_pipe->end_point, usb_pipe->device, usb_pipe->type, usb_pipe->direction )
  #endif
  // prepare characteristic
  uint32_t characteristic = HCD_DWHCI_CHAN_CHARACTER_DEVICE_ADDRESS( usb_pipe->device )
    | HCD_DWHCI_CHAN_CHARACTER_END_POINT_NUMBER( usb_pipe->end_point )
    | HCD_DWHCI_CHAN_CHARACTER_END_POINT_DIRECTION( usb_pipe->direction )
    | HCD_DWHCI_CHAN_CHARACTER_LOW_SPEED( ( usb_pipe->speed == LIBUSB_SPEED_LOW ? 1 : 0 ) )
    | HCD_DWHCI_CHAN_CHARACTER_TYPE( usb_pipe->type )
    | HCD_DWHCI_CHAN_CHARACTER_MAXIMUM_PACKET_SIZE( usb_number_from_packet_size( usb_pipe->max_size ) )
    | HCD_DWHCI_CHAN_CHARACTER_ENABLE( 0 )
    | HCD_DWHCI_CHAN_CHARACTER_DISABLE( 0 );
  // prepare split control
  uint32_t split_control = 0;
  if ( DWHCI_SPLIT_PHASE_NONE != entry->split_phase ) {
    split_control |= HCD_DWHCI_CHAN_SPLIT_CONTROL_SPLIT_ENABLE( 1 )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_HUB_ADDRESS( parent_device_number )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_PORT_ADDRESS( port_number + 1 )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_EXTRACT_TRANSACTION_POSITION( 3 ); /// FIXME: NOT CORRECT IN ALL CASES
    if ( DWHCI_SPLIT_PHASE_CSPLIT == entry->split_phase ) {
      split_control |= HCD_DWHCI_CHAN_SPLIT_CONTROL_COMPLETE_SPLIT( 1 );
    }
  }
  // evaluate paket count
  uint32_t packet_count = ( buffer_length + 7 ) / 8;
  if ( LIBUSB_SPEED_LOW != usb_pipe->speed ) {
    packet_count = (
      buffer_length + usb_number_from_packet_size( usb_pipe->max_size ) - 1 ) / usb_number_from_packet_size( usb_pipe->max_size );
  }
  if ( 0 == packet_count ) {
    packet_count = 1;
  }
  const uint32_t original_packet_count = packet_count;
  // reset packet count and buffer_size for data to one packet at the time
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "packet_count = %"PRIu32", buffer_length = %"PRIu32"\r\n",
      packet_count, buffer_length )
  #endif
  if ( DWHCI_QUEUE_CHANNEL_STATUS_DATA == entry->status ) {
    packet_count = 1;
    if ( buffer_length > usb_number_from_packet_size( usb_pipe->max_size ) ) {
      buffer_length = usb_number_from_packet_size( usb_pipe->max_size );
    }
    entry->buffer_size_to_transfer = buffer_length;
  }
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "packet_count = %"PRIu32", buffer_length = %"PRIu32"\r\n",
      packet_count, buffer_length )
  #endif
  // prepare transfer data
  uint32_t transfer_data = 0;
  if ( ! channel_prepared ) {
    // set transfer size and packet id
    transfer_data = HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( buffer_length )
      | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_ID( packet_id );
  } else {
    transfer_data = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ) );
    // set transfer size
    transfer_data &= ~HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE_MASK;
    transfer_data |= HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( buffer_length );
  }
  // set packet count
  transfer_data |= HCD_DWHCI_CHAN_XFER_SIZE_PACKET_COUNT( packet_count );
  // set packet size and count if not set
  if ( 0 == entry->packets_to_transfer ) {
    entry->packets_to_transfer = original_packet_count;
    entry->packet_size = usb_number_from_packet_size( usb_pipe->max_size );
    // debug output
    #if defined ( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "packet_size = %"PRIu32", transfer count = %"PRIu32"\r\n",
        entry->packet_size, entry->packets_to_transfer )
    #endif
  }
  // interrupts are handled differently and block the channel permanently
  if ( LIBUSB_TRANSFER_INTERRUPT == usb_pipe->type ) {
    const uint32_t current_frame = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
    // determine interval
    uint32_t calculated_interval = interval;
    if ( LIBUSB_SPEED_HIGH == usb_pipe->speed ) {
      const uint32_t micro_frames = 1U << ( interval - 1 );
      calculated_interval = ( micro_frames + 7U ) / 8U;
    }
    // get next frame
    const uint32_t target_frame = (current_frame & 0xffff) + calculated_interval;
    // add odd frame bit depending on target frame
    characteristic |= ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ODD_FRAME( target_frame & 0x1 ? 1 : 0 );
  }
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
  // write characteristics
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel );
  sequence[ 0 ].value = characteristic;
  // write split control
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel );
  sequence[ 1 ].value = split_control;
  // set transfer data
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel );
  sequence[ 2 ].value = transfer_data;
  // write to io
  const int result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Prepare channel sequence failed: %s\r\n",
        strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "characteristic = %#"PRIx32", split_control = %#"PRIx32", transfer_data = %#"PRIx32", packet_count = %#"PRIx32"\r\n",
      characteristic, split_control, transfer_data, packet_count )
  #endif
  // free sequence
  iomem_release_mmio_sequence( sequence );
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
  #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
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
  if ( ! entry ) {
    // some debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate entry for queue\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out everything
  memset( entry, 0, sizeof( *entry ) );
  // duplicate data
  void* dup_data = malloc( size );
  if ( ! dup_data ) {
    // some debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to duplicate data\r\n" )
    #endif
    // free entry
    free( entry );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out dup data and copy over co
  memset( dup_data, 0, size );
  memcpy( dup_data, data, size );
  // prepare entry
  entry->data = dup_data;
  entry->data_size = size;
  entry->status = status;
  entry->error = LIBUSB_TRANSFER_ERROR_NO_ERROR;
  // map buffer
  entry->buffer = mmap( nullptr, size, PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_BUS | MAP_DEVICE , -1, 0 );
  // handle map failed
  if ( MAP_FAILED == entry->buffer ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate buffer\r\n" )
    #endif
    // free entry again
    free( entry );
    free( dup_data );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out buffer
  memset( entry->buffer, 0, size );
  // queue entry
  dwhci_queue_queue_entry( entry );
  // handle push to out
  if ( out ) {
    *out = entry;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn void dwhci_queue_queue_entry(channel_queue_entry_t*)
 * @brief Push entry into queue
 * @param entry
 */
void dwhci_queue_queue_entry( channel_queue_entry_t* entry ) {
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
}

/**
 * @fn response_t dwhci_queue_remove_entry(channel_queue_entry_t*, bool)
 * @brief Remove given entry from queue
 * @param entry entry to remove
 * @param free_up free up space
 * @return
 */
response_t dwhci_queue_remove_entry( channel_queue_entry_t* entry, const bool free_up ) {
  // validate data
  if ( ! entry ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid entry passed for removal\r\n" )
    #endif
    // return einval
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // handle free of memo
  if ( free_up ) {
    // handle buffer
    if ( entry->buffer ) {
      munmap( entry->buffer, entry->data_size );
    }
    if ( entry->message ) {
      free( entry->message );
    }
    // handle data
    if ( entry->data ) {
      free( entry->data );
    }
  }
  // handle first element
  if ( entry == configuration.list ) {
    // set list to next
    configuration.list = entry->next;
    // handle list valid
    if ( configuration.list ) {
      // set prev of list to nullptr
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
  #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
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
      #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
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
response_t dwhci_channel_send_async_start_channel( channel_queue_entry_t* entry ) {
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Translate buffer to physical\r\n" )
  #endif
  // translate buffer to physical bus address
  const uintptr_t phys = _syscall_memory_translate_bus(
    ( uintptr_t )entry->buffer, entry->data_size );
  if ( errno ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
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
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( 8, &sequence_size );
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
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ_OR;
  sequence[ 4 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 5 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
  sequence[ 5 ].value = 1U << entry->channel;
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 6 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
  sequence[ 6 ].value = ~HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 );
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 7 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
  sequence[ 7 ].value = HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
  // execute sequence
  if ( 0 != iomem_execute_sequence( fd_iomem, sequence, sequence_size ) ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to disable channel interrupt\r\n" )
    #endif
    // release sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // release sequence
  iomem_release_mmio_sequence( sequence );
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "channel = %"PRIu8"\r\n", entry->channel )
    EARLY_STARTUP_PRINT( "Done\r\n" )
  #endif
  // set time
  if ( DWHCI_QUEUE_POLL_STATUS_DATA == entry->status ) {
    entry->poll_last_timer = _syscall_timer_tick_count();
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_async_stop_channel(const channel_queue_entry_t*, bool)
 * @brief
 * @param entry
 * @param free_channel free da channel
 * @return
 */
response_t dwhci_channel_send_async_stop_channel( const channel_queue_entry_t* entry, const bool free_channel ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Stopping channel via mmio sequence\r\n" )
  #endif
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = iomem_prepare_mmio_sequence( free_channel ? 4 : 2, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // prepare sequence
  // only if channel is freed
  if ( free_channel ) {
    // read all chan int mask
    sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
    sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
    // disable channel with write back
    sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
    sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_ALLCHAN_INT_MASK;
    sequence[ 1 ].value = ~(1U << entry->channel);
    // reset enable bit with read
    sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ;
    sequence[ 2 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
    // set disable bit with write
    sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
    sequence[ 3 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
    sequence[ 3 ].value = HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 ) | HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
  } else {
    // load channel characteristics
    sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
    sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
    // set disable and enable bit to 1
    sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
    sequence[ 1 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( entry->channel );
    sequence[ 1 ].value = HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 )
      | HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
  }
  // execute sequence
  if ( 0 != iomem_execute_sequence( fd_iomem, sequence, sequence_size ) ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to disable channel interrupt\r\n" )
    #endif
    // release sequence
    iomem_release_mmio_sequence( sequence );
    // return io error
    return HCD_RESPONSE_ERROR_IO;
  }
  // release sequence
  iomem_release_mmio_sequence( sequence );
  // free channel if set
  if ( free_channel ) {
    // free allocated channel
    const response_t result = dwhci_free_channel( entry->channel );
    if ( HCD_RESPONSE_OK != result ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to free channel %"PRIu8"\r\n", entry->channel )
      #endif
      // return result
      return result;
    }
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
  const usb_control_message_t* entry_data = entry->data;
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
  #if defined( DWHCI_ENABLE_DEBUG )
    if ( entry->buffer_offset == 0 ) {
      auto const setup = (uint8_t*)entry->buffer;
      EARLY_STARTUP_PRINT(
        "SETUP: %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        setup[0], setup[1], setup[2], setup[3],
        setup[4], setup[5], setup[6], setup[7]
      )
    }
  #endif
  // set buffer size
  entry->buffer_size_to_transfer = sizeof( libusb_device_request_t ) - entry->buffer_offset;
  // prepare channel
  const response_t result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    sizeof( libusb_device_request_t ) - entry->buffer_offset,
    DWHCI_CHANNEL_STATE_SETUP,
    &setup_pipe,
    0,
    false,
    entry
  );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
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
  const usb_control_message_t* entry_data = entry->data;
  // handle no data to transmit or receive
  if (0 == entry_data->buffer_length ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No data to send continue with ack\r\n" )
    #endif
    // switch to next state
    entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
    // continue directly
    return dwhci_channel_async_continue( entry );
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
  // set buffer size
  entry->buffer_size_to_transfer = entry_data->buffer_length - entry->buffer_offset;
  // prepare channel
  response_t result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    entry_data->buffer_length - entry->buffer_offset,
    entry->channel_data_state,
    &data_pipe,
    0,
    false,
    entry
  );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
  // start send data packet
  result = dwhci_channel_send_async_start_channel( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to start async channel\r\n" )
    #endif
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
  usb_control_message_t* entry_data = entry->data;
  // populate last transfer and data
  if ( DWHCI_QUEUE_CHANNEL_STATUS_DATA == entry->previous_status ) {
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
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "entry->transferred = %"PRIu32", entry_data->buffer_length = %zu\r\n",
          entry->transferred, entry_data->buffer_length );
      #endif
      // copy back data
      memcpy( entry_data->buffer, entry->buffer, entry_data->last_transfer );
    } else {
      entry_data->last_transfer = entry_data->buffer_length;
    }
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
  // set buffer size
  entry->buffer_size_to_transfer = 0;
  // prepare channel
  response_t result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    0,
    DWHCI_CHANNEL_STATE_DATA1,
    &ack_pipe,
    0,
    false,
    entry
  );
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to prepare allocated channel\r\n" )
    #endif
    // return result
    return result;
  }
  // start send data packet
  result = dwhci_channel_send_async_start_channel( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to start async channel\r\n" )
    #endif
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
  // handle transfer size
  if ( entry->transferred ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Warning non zero status transfer: %"PRIu32"\r\n", entry->transferred )
    #endif
  }
  auto const message = ( usbd_control_message_t* )entry->message;
  auto const entry_data = ( usb_control_message_t* )entry->data;
  // handle timer
  if ( entry->timer ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Clearing timeout\r\n" )
    #endif
    // clear tim
    _syscall_timer_release( entry->timer );
  }
  // attach shared memory
  void* shm = _syscall_memory_shared_attach( message->shm_id, 0 );
  if ( errno ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to attach shared memory again\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // stop transmission
  response_t result = dwhci_channel_send_async_stop_channel( entry, true );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to stop channel\r\n")
    #endif
    // return result
    return result;
  }
  // finally set no error
  if ( entry->error ) {
    entry->error |= LIBUSB_TRANSFER_ERROR_PROCESSING;
  }
  // populate error
  entry_data->error = entry->error;
  // copy over to shared memory
  memcpy( shm, entry->data, entry->data_size );
  // allocate response structure
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( usbd_control_message_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate memory for response\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out memory
  memset( response, 0, response_size );
  // detach shared memory
  _syscall_memory_shared_detach( message->shm_id );
  // populate response
  memcpy( response->container, entry->message, sizeof( usbd_control_message_t ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, entry->response_info );
  // free entry
  free( response );
  // return with next entry
  result = dwhci_continue_next( entry );
  if ( HCD_RESPONSE_OK != result ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to continue with next request\r\n" )
    #endif
  }
  // destroy queue entry
  result = dwhci_queue_remove_entry( entry, true );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to remove entry\r\n" )
    #endif
    // return result
    return result;
  }
  // continue with next
  return dwhci_continue_next( nullptr );
}

/**
 * @fn response_t dwhci_channel_send_cancel(const channel_queue_entry_t*)
 * @brief initiates cancellation of entry
 * @param entry
 * @return
 */
response_t dwhci_channel_send_cancel( const channel_queue_entry_t* entry ) {
  // handle not correct status
  if (
    entry->status != DWHCI_QUEUE_CHANNEL_STATUS_SETUP
    && entry->status != DWHCI_QUEUE_CHANNEL_STATUS_DATA
    && entry->status != DWHCI_QUEUE_CHANNEL_STATUS_ACK
    && entry->status != DWHCI_QUEUE_POLL_STATUS_DATA
  ) {
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // stop transmission
  const response_t result = dwhci_channel_send_async_stop_channel( entry, false );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to stop channel\r\n")
    #endif
    // return result
    return result;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_send_cancel_done(channel_queue_entry_t*)
 * @brief send cancellation done
 * @param entry
 * @return
 */
response_t dwhci_channel_send_cancel_done( channel_queue_entry_t* entry ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Handling cancellation done \r\n" )
  #endif
  auto const message = ( usbd_control_message_t* )entry->message;
  auto const entry_data = ( usb_control_message_t* )entry->data;
  // stop transmission
  response_t result = dwhci_channel_send_async_stop_channel( entry, true );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to stop channel\r\n")
    #endif
    // return result
    return result;
  }
  // set error
  entry->error = LIBUSB_TRANSFER_ERROR_TIMEOUT | LIBUSB_TRANSFER_ERROR_PROCESSING;
  // populate error
  entry_data->error = entry->error;
  // allocate response structure
  constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( usbd_control_message_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate memory for response\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // clear out memory
  memset( response, 0, response_size );
  // detach shared memory
  _syscall_memory_shared_detach( message->shm_id );
  // populate response
  memcpy( response->container, entry->message, sizeof( usbd_control_message_t ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, entry->response_info );
  // free entry
  free( response );
  // continue with next one
  result = dwhci_continue_next( entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to continue with next request\r\n" )
    #endif
    // return result
    return result;
  }
  // destroy queue entry
  result = dwhci_queue_remove_entry( entry, true );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to remove entry\r\n" )
    #endif
    // return result
    return result;
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_channel_async_continue(channel_queue_entry_t*)
 * @brief Method to start entry transfer depending on status
 * @param entry
 * @return
 */
response_t dwhci_channel_async_continue( channel_queue_entry_t* entry ) {
  #if defined ( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Continuing with channel %"PRIu8"\r\n", entry->channel )
  #endif
  // continue channel depending on status
  switch ( entry->status ) {
    // control packages
    case DWHCI_QUEUE_CHANNEL_STATUS_SETUP:
      return dwhci_channel_send_async_setup( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_DATA:
      return dwhci_channel_send_async_data( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_ACK:
      return dwhci_channel_send_async_ack( entry );
    case DWHCI_QUEUE_CHANNEL_STATUS_DONE:
      return dwhci_channel_send_async_done( entry );
    // polling related
    case DWHCI_QUEUE_POLL_STATUS_DATA:
      return dwhci_channel_poll_async_data( entry );
    case DWHCI_QUEUE_POLL_STATUS_ACK:
      return dwhci_channel_poll_async_ack( entry );
    case DWHCI_QUEUE_POLL_STATUS_DONE:
      return dwhci_channel_poll_async_done( entry );
    case DWHCI_QUEUE_POLL_STATUS_WAIT:
      return HCD_RESPONSE_OK;
    // cancellation
    case DWHCI_QUEUE_CANCEL:
      return dwhci_channel_send_cancel( entry );
    case DWHCI_QUEUE_CANCEL_DONE:
      return dwhci_channel_send_cancel_done( entry );
    default:
      return HCD_RESPONSE_ERROR_UNKNOWN;
  }
}

/**
 * @fn response_t dwhci_channel_send_async(usb_control_message_t*, size_t, const usbd_control_message_t*, size_t);
 * @brief Wrapper to perform async channel send
 * @param data data to send
 * @param data_size data size
 * @param message original message
 * @param response_info where to respond result to
 * @return
 */
response_t dwhci_channel_send_async(
  usb_control_message_t* data,
  const size_t data_size,
  const usbd_control_message_t* message,
  const size_t response_info
) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT("Channel send async\r\n")
  #endif
  // push data with channel to queue
  channel_queue_entry_t* entry = nullptr;
  response_t result = dwhci_queue_add_entry( data, data_size, DWHCI_QUEUE_CHANNEL_STATUS_PENDING, &entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to add request to queue\r\n" )
    #endif
    // return result
    return result;
  }
  // duplicate message
  usbd_control_message_t* dup_message = malloc( sizeof( *dup_message ) );
  if ( ! dup_message ) {
    // clear entry again
    dwhci_queue_remove_entry( entry, true );
    // return no memory
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  memcpy( dup_message, message, sizeof( *dup_message ) );
  // populate entry
  entry->response_info = response_info;
  entry->message = dup_message;
  // initialize split phase
  entry->split_phase = LIBUSB_SPEED_HIGH != data->pipe_address.speed ? DWHCI_SPLIT_PHASE_SSPLIT : DWHCI_SPLIT_PHASE_NONE;
  entry->channel_data_state = DWHCI_CHANNEL_STATE_DATA1;
  // try to allocate a channel
  uint8_t channel = 0;
  result = dwhci_allocate_channel( &channel );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to enable channel interrupt\r\n" )
    #endif
    // free channel again
    dwhci_free_channel( channel );
    // remove from queue again
    dwhci_queue_remove_entry( entry, true );
    // return result
    return result;
  }
  // kickstart timeout if set
  if ( data->timeout ) {
    // acquire timeout
    entry->timer = timer_acquire( data->timeout );
    // handle error
    if ( errno ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to acquire timeout\r\n" )
      #endif
      // free channel again
      dwhci_free_channel( channel );
      // remove from queue again
      dwhci_queue_remove_entry( entry, true );
      // return error
      return HCD_RESPONSE_ERROR_IO;
    }
  }
  // continue async
  return dwhci_channel_async_continue( entry );
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
  const usb_interrupt_poll_t* entry_data = entry->data;
  // handle no data to transmit or receive
  if ( 0 == entry_data->buffer_length ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "No data to send continue with ack\r\n" )
    #endif
    // switch to next state
    entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
    // continue directly
    return dwhci_channel_async_continue( entry );
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
  // prepare channel
  const response_t result = dwhci_prepare_channel(
    entry_data->parent_device_number,
    entry_data->port_number,
    entry->channel,
    entry_data->buffer_length - entry->buffer_offset,
    entry->poll_state,
    &data_pipe,
    entry_data->interval,
    entry->prepared,
    entry
  );
  // set buffer size
  entry->buffer_size_to_transfer = entry_data->buffer_length - entry->buffer_offset;
  // handle error
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
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
  usb_interrupt_poll_t* entry_data = entry->data;
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
  entry->status = DWHCI_QUEUE_POLL_STATUS_DONE;
  return dwhci_channel_async_continue( entry );
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
  // set error processing if error occurred
  if ( entry->error ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Error: %#x\r\n", entry->error )
    #endif
    // error entry
    entry->error |= LIBUSB_TRANSFER_ERROR_PROCESSING;
  }
  usb_interrupt_poll_t* entry_data = entry->data;
  entry_data->error = entry->error;
  // only send on not nack
  if ( ! ( entry->error & LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE ) ) {
    // debug output
    //#if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "DATA\r\n" )
    //#endif
    // allocate response structure
    const size_t response_size = sizeof( vfs_ioctl_perform_response_t ) + sizeof( usbd_interrupt_return_t )
      + sizeof( char ) * entry_data->last_transfer;
    vfs_ioctl_perform_response_t* response = malloc( response_size );
    if ( ! response ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate memory for response\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_MEMORY;
    }
    // clear out memory
    memset( response, 0, response_size );
    // populate container
    auto const container = ( usbd_interrupt_return_t* )response->container;
    container->device_number = entry_data->device_number;
    container->length = sizeof( char ) * entry_data->last_transfer;
    container->error = entry->error;
    memcpy( container->buffer, entry->buffer, entry_data->last_transfer );
    // raise async with fire and forget
    bolthur_rpc_raise_generic(
      GENERIC_POLL_INTERRUPT,
      entry->origin,
      response,
      response_size,
      nullptr,
      GENERIC_POLL_INTERRUPT,
      response,
      response_size,
      0,
      0,
      nullptr,
      true,
      true
    );
    // free entry
    free( response );
  }
  // reset entry partly
  entry->prepared = true;
  entry->buffer_offset = 0;
  entry->buffer_size_to_transfer = entry_data->buffer_length;
  memset( entry->buffer, 0, entry_data->buffer_length );
  // check interval
  size_t wait_time = 0;
  if ( entry->poll_last_timer > 0 ) {
    // get frequency and current tick count
    const size_t frequency = _syscall_timer_frequency();
    const size_t current_tick_count = _syscall_timer_tick_count();
    // calculate difference and finally passed milliseconds
    const size_t difference = current_tick_count - entry->poll_last_timer;
    const size_t passed_milliseconds = ( size_t )( ( ( double )difference / ( double )frequency ) * 1000.0 );
    // handle not enough time in between => wait
    if ( passed_milliseconds < entry->interval ) {
      wait_time = entry->interval - passed_milliseconds;
      entry->status = DWHCI_QUEUE_POLL_STATUS_WAIT;
    } else {
      entry->status = DWHCI_QUEUE_POLL_STATUS_DATA;
    }
    EARLY_STARTUP_PRINT( "interval: %"PRIu32", passed_milliseconds: %zu\r\n",
      entry->interval, passed_milliseconds )
    entry->poll_last_timer = 0;
  } else {
    // next step is poll data
    entry->status = DWHCI_QUEUE_POLL_STATUS_DATA;
  }
  // handle stall by cancelling
  if ( entry->error & LIBUSB_TRANSFER_ERROR_STALL ) {
    EARLY_STARTUP_PRINT( "STALL ERROR\r\n" )
    entry->status = DWHCI_QUEUE_CANCEL;
  } else {
    entry->error = 0;
  }
  EARLY_STARTUP_PRINT( "entry->status = %d\r\n", entry->status )
  // handle wait
  if ( wait_time > 0 ) {
    // acquire timeout
    entry->poll_timer_id = timer_acquire( wait_time );
    // handle error
    if ( errno ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to acquire timeout\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_IO;
    }
  }
  // continue with next
  return dwhci_continue_next( entry );
}

/**
 * @fn response_t dwhci_channel_poll_async(usb_interrupt_poll_t*, size_t, usbd_interrupt_message_t*, size_t);
 * @brief Wrapper to perform async channel polling
 * @param data data to be used for polling
 * @param data_size data size
 * @param message original message
 * @param origin process to contact in terms of completeness
 * @return
 */
response_t dwhci_channel_poll_async(
  usb_interrupt_poll_t* data,
  const size_t data_size,
  const usbd_interrupt_message_t* message,
  const pid_t origin
) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT("Channel send async\r\n")
  #endif
  // check if already in
  auto current = configuration.list;
  while ( current ) {
    // get entry data
    const usb_interrupt_poll_t* entry_data = current->data;
    // handle already in
    if (
      entry_data->device_number == data->device_number
      && entry_data->parent_device_number == data->parent_device_number
      && entry_data->direction == data->direction
      && entry_data->pipe_address.end_point == data->pipe_address.end_point
    ) {
      // allocate response structure
      constexpr size_t response_size = sizeof( vfs_ioctl_perform_response_t )
        + sizeof( usbd_interrupt_return_t );
      // allocate space for return
      vfs_ioctl_perform_response_t* response = malloc( response_size );
      if ( ! response ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to allocate memory for response\r\n" )
        #endif
        // return error
        return HCD_RESPONSE_ERROR_MEMORY;
      }
      // clear out memory
      memset( response, 0, response_size );
      // populate container
      auto const container = ( usbd_interrupt_return_t* )response->container;
      container->device_number = entry_data->device_number;
      container->length = 0;
      container->error = data->error;
      response->status = -EALREADY;
      // raise async with fire and forget
      bolthur_rpc_raise_generic(
        GENERIC_POLL_INTERRUPT,
        origin,
        response,
        response_size,
        nullptr,
        GENERIC_POLL_INTERRUPT,
        response,
        response_size,
        0,
        0,
        nullptr,
        true,
        true
      );
      // return success
      return HCD_RESPONSE_OK;
    }
    // switch to next
    current = current->next;
  }
  // push data with channel to queue
  channel_queue_entry_t* entry = nullptr;
  response_t result = dwhci_queue_add_entry( data, data_size, DWHCI_QUEUE_POLL_STATUS_PENDING, &entry );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to add request to queue\r\n" )
    #endif
    // return result
    return result;
  }
  // duplicate message
  usbd_interrupt_message_t* dup_message = malloc( sizeof( *dup_message ) );
  if ( ! dup_message ) {
    // clear entry again
    dwhci_queue_remove_entry( entry, true );
    // return no memory
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  memcpy( dup_message, message, sizeof( *dup_message ) );
  // populate entry
  entry->origin = origin;
  entry->message = dup_message;
  entry->interval = data->interval;
  entry->poll_state = DWHCI_CHANNEL_STATE_DATA0;
  // initialize split phase
  entry->split_phase = LIBUSB_SPEED_HIGH != data->pipe_address.speed ? DWHCI_SPLIT_PHASE_SSPLIT : DWHCI_SPLIT_PHASE_NONE;
  // try to allocate a channel
  uint8_t channel = 0;
  result = dwhci_allocate_channel( &channel );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to enable channel interrupt\r\n" )
    #endif
    // free channel again
    dwhci_free_channel( channel );
    // remove from queue again
    dwhci_queue_remove_entry( entry, true );
    // return result
    return result;
  }
  // continue async
  return dwhci_channel_async_continue( entry );
}

/**
 * @fn response_t dwhci_continue_next(channel_queue_entry_t*)
 * @brief Continue with next entry
 * @param current current command / poll sequence
 * @return
 */
response_t dwhci_continue_next( channel_queue_entry_t* current ) {
  // get next entry
  channel_queue_entry_t* out;
  response_t result = dwhci_get_next_entry( &out );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to get next entry\r\n" )
    #endif
    // return result
    return result;
  }
  // handle out
  if ( out ) {
    // push current to pending
    if (
      current
      && (
        DWHCI_QUEUE_POLL_STATUS_DATA == current->status
        || DWHCI_QUEUE_POLL_STATUS_WAIT == current->status
      )
    ) {
      // set status back to pending when not waiting and prepared to false
      if ( DWHCI_QUEUE_POLL_STATUS_DATA == current->status ) {
        current->status = DWHCI_QUEUE_POLL_STATUS_PENDING;
      }
      current->prepared = false;
      // remove entry from queue
      result = dwhci_queue_remove_entry( current, false );
      if ( HCD_RESPONSE_OK != result ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to remove entry from list\r\n" )
        #endif
        // return result
        return result;
      }
      // queue again at the end
      dwhci_queue_queue_entry( current );
      // free channel for uniform startup of poll / command
      result = dwhci_free_channel( current->channel );
      // handle error
      if ( HCD_RESPONSE_OK != result ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to free channel\r\n" )
        #endif
        // return result
        return result;
      }
    }
    // try to allocate a channel
    uint8_t channel = 0;
    result = dwhci_allocate_channel( &channel );
    if ( HCD_RESPONSE_OK != result ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to allocate a channel, entry is queued\r\n" )
      #endif
      // return error
      return result;
    }
    // set status of out and channel
    out->status = DWHCI_QUEUE_POLL_STATUS_PENDING == out->status
      ? DWHCI_QUEUE_POLL_STATUS_DATA
      : DWHCI_QUEUE_CHANNEL_STATUS_SETUP;
    // set channel of out
    out->channel = channel;
    // enable channel interrupt for setup commands
    if ( DWHCI_QUEUE_POLL_STATUS_PENDING != current->status ) {
      // enable channel interrupt
      result = dwhci_enable_channel_interrupt( channel );
      // handle error
      if ( HCD_RESPONSE_OK != result ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to enable channel interrupt\r\n" )
        #endif
        // free channel again
        dwhci_free_channel( channel );
        // set back to pending
        out->status = DWHCI_QUEUE_CHANNEL_STATUS_SETUP == out->status
          ? DWHCI_QUEUE_CHANNEL_STATUS_PENDING
          : DWHCI_QUEUE_POLL_STATUS_PENDING;
        // return result
        return result;
      }
    }
    // continue with it
    return dwhci_channel_async_continue( out );
  }
  // handle polling => just continue
  if ( current && DWHCI_QUEUE_POLL_STATUS_DATA == current->status ) {
    // continue polling
    return dwhci_channel_async_continue( current );
  }
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_get_next_entry(channel_queue_entry_t**)
 * @brief Function to get next entry to execute
 * @param out out pointer
 * @return
 */
response_t dwhci_get_next_entry( channel_queue_entry_t** out ) {
  // validate parameter
  if ( ! out ) {
    return HCD_RESPONSE_ERROR_EINVAL;
  }
  // try to get next queued setup entry
  auto current = configuration.list;
  // loop through list
  while ( current ) {
    // handle pending setup
    if ( DWHCI_QUEUE_CHANNEL_STATUS_PENDING == current->status ) {
      *out = current;
      return HCD_RESPONSE_OK;
    }
    // go to next
    current = current->next;
  }
  // reset current
  current = configuration.list;
  // loop through list
  while ( current ) {
    if ( DWHCI_QUEUE_POLL_STATUS_PENDING == current->status ) {
      *out = current;
      return HCD_RESPONSE_OK;
    }
    // go to next
    current = current->next;
  }
  // nothing to continue with
  *out = nullptr;
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_cancel_by_device(uint32_t)
 * @brief Cancel running stuff by device
 * @param device_number
 * @return
 */
response_t dwhci_cancel_by_device( const uint32_t device_number ) {
  // get list
  auto current = configuration.list;
  // iterate and cancel
  while ( current ) {
    // get is poll flag
    const bool is_poll = current->status == DWHCI_QUEUE_POLL_STATUS_PENDING
      || current->status == DWHCI_QUEUE_POLL_STATUS_DATA
      || current->status == DWHCI_QUEUE_POLL_STATUS_ACK
      || current->status == DWHCI_QUEUE_POLL_STATUS_DONE;
    // get is command flag
    const bool is_command = current->status == DWHCI_QUEUE_CHANNEL_STATUS_PENDING
      || current->status == DWHCI_QUEUE_CHANNEL_STATUS_SETUP
      || current->status == DWHCI_QUEUE_CHANNEL_STATUS_DATA
      || current->status == DWHCI_QUEUE_CHANNEL_STATUS_ACK
      || current->status == DWHCI_QUEUE_CHANNEL_STATUS_DONE;
    // handle skip
    if (
      (
        is_poll
        && ( ( usb_interrupt_poll_t* )current->data )->device_number != device_number
      ) || (
        is_command
        && ( ( usb_control_message_t* )current->data )->device_number != device_number
      )
    ) {
      // go to next
      current = current->next;
      // skip rest
      continue;
    }
    // handle pending
    if (
      DWHCI_QUEUE_CHANNEL_STATUS_PENDING == current->status
      || DWHCI_QUEUE_POLL_STATUS_PENDING == current->status
    ) {
      // get entry to delete
      auto to_delete = current;
      // switch to next
      current = current->next;
      // remove with cleanup
      dwhci_queue_remove_entry( to_delete, true );
      // skip rest
      continue;
    }
    // cancel
    current->status = DWHCI_QUEUE_CANCEL;
    // start cancellation
    const response_t response = dwhci_channel_async_continue( current );
    if ( HCD_RESPONSE_OK != response ) {
      // debug output
      #if defined ( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Unable to start cancellation process\r\n" )
      #endif
      // return response
      return response;
    }
    // go to next
    current = current->next;
  }
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
    iomem_mailbox_release( request );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // handle not successful
  if ( MAILBOX_REQUEST_SUCCESSFUL != ( uint32_t )request[ 1 ] ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Request not successful: %#"PRIx32"\r\n",
        ( uint32_t )request[ 1 ] )
    #endif
    // free request
    iomem_mailbox_release( request );
    // return error
    return HCD_RESPONSE_ERROR_MAILBOX;
  }
  // handle invalid device id returned
  if ( MAILBOX_POWER_STATE_DEVICE_USB_HCD != request[ 5 ] ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Invalid device id returned, expected %#x but received %#"PRIX32"\r\n",
        MAILBOX_POWER_STATE_DEVICE_USB_HCD, request[ 5 ] )
    #endif
    // free
    iomem_mailbox_release( request );
    // return error
    return HCD_RESPONSE_ERROR_MAILBOX;
  }
  // check for powered on correctly
  if ( ( request[ 6 ] & 0x3 ) != MAILBOX_SET_POWER_STATE_ON ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT(
        "Device not powered on successfully: %#"PRIx32"\r\n",
        request[ 6 ] & 0x3 )
    #endif
    // free
    iomem_mailbox_release( request );
    // return error
    return HCD_RESPONSE_ERROR_MAILBOX;
  }
  // free
  iomem_mailbox_release( request );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Flush tx fifo failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
    iomem_release_mmio_sequence( sequence );
    // return error
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence again
  iomem_release_mmio_sequence( sequence );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "flush rx fifo failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
    iomem_release_mmio_sequence( sequence );
    // return error
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence again
  iomem_release_mmio_sequence( sequence );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Querying vendor information failed: %s\r\n",
        strerror( e ) )
    #endif
    // close file descriptor
    close( fd_iomem );
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
  iomem_release_mmio_sequence( sequence );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Disable of interrupts failed: %s\r\n",
        strerror( e ) )
    #endif
    // close file descriptor
    close( fd_iomem );
    // free sequence
    iomem_release_mmio_sequence( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  iomem_release_mmio_sequence( sequence );
  // power on usb hub
  response_t dwhci_result = dwhci_power_on();
  // handle error
  if ( HCD_RESPONSE_OK != dwhci_result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
    iomem_release_mmio_sequence( sequence );
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
    iomem_release_mmio_sequence( sequence );
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
  uint32_t usb_cfg = mmio_read( PERIPHERAL_DWHCI_CORE_USB_CFG );
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
  mmio_write( PERIPHERAL_DWHCI_CORE_USB_CFG, usb_cfg );

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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 1 ].value = ~HCD_DWHCI_CORE_USB_CFG_SRP_CAPABLE;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_INT_STAT;
  sequence[ 2 ].value = -1U;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Reset sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // release sequence
  iomem_release_mmio_sequence( sequence );

  dwhci_core_flush_tx_fifo(0x10);
  dwhci_core_flush_rx_fifo();

  // read out host config
  host_cfg = mmio_read( PERIPHERAL_DWHCI_HOST_CFG );
  // put channels into known states
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
      #if defined( DWHCI_ENABLE_DEBUG )
        const int e = errno;
        EARLY_STARTUP_PRINT( "host config sequence failed: %s\r\n", strerror( e ) )
      #endif
      // free sequence
      iomem_release_mmio_sequence( sequence );
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
      #if defined( DWHCI_ENABLE_DEBUG )
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
        EARLY_STARTUP_PRINT( "Unable to clear channel %"PRIu32"\r\n", channel )
      #endif
    }
  }
  // free sequence
  iomem_release_mmio_sequence( sequence );

  uint32_t host_port = mmio_read( PERIPHERAL_DWHCI_HOST_PORT );
  if ( ! ( host_port & HCD_DWHCI_HOST_PORT_POWER ) ) {
    host_port |= HCD_DWHCI_HOST_PORT_POWER;
    mmio_write( PERIPHERAL_DWHCI_HOST_PORT, host_port );
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
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "Enable of interrupts failed: %s\r\n", strerror( e ) )
    #endif
    // close file descriptor
    close( fd_iomem );
    // free sequence
    iomem_release_mmio_sequence( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  iomem_release_mmio_sequence( sequence );

  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Power on root port\r\n" )
  #endif
  // read host port
  host_port = mmio_read( PERIPHERAL_DWHCI_HOST_PORT );
  // reset changed bits
  host_port &= ~( uint32_t )(
    HCD_DWHCI_HOST_PORT_CONNECT_CHANGED
    | HCD_DWHCI_HOST_PORT_ENABLE_CHANGED
    | HCD_DWHCI_HOST_PORT_OVERCURRENT_CHANGED
  );
  // set over current changed
  host_port |= HCD_DWHCI_HOST_PORT_POWER;
  // write back host port
  mmio_write( PERIPHERAL_DWHCI_HOST_PORT, host_port );
  constexpr long milliseconds = 100;
  struct timespec ts = {
    .tv_sec = milliseconds / 1000,
    .tv_nsec = ( milliseconds % 1000 ) * 1000000
  };
  // sleep a bit
  nanosleep( &ts, &ts );

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
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 6 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 6 ].value = ~HCD_DWHCI_HOST_PORT_RESET;
  // delay again for 20ms
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 7 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 7 ].sleep = 20;
  // perform request
  result = iomem_execute_sequence( fd_iomem, sequence, sequence_size );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      const int e = errno;
      EARLY_STARTUP_PRINT( "host config sequence failed: %s\r\n", strerror( e ) )
    #endif
    // free sequence
    iomem_release_mmio_sequence( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  iomem_release_mmio_sequence( sequence );

  // read interrupt register
  const uint32_t interrupt = mmio_read( PERIPHERAL_DWHCI_CORE_INT_STAT );
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
  #endif
  // mask pending interrupts
  mmio_write( ( uint32_t )PERIPHERAL_DWHCI_CORE_INT_STAT, interrupt );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Acquiring interrupt %d\r\n", ARM_IRQ_USB )
  #endif
  // register interrupt
  _syscall_interrupt_acquire( ARM_IRQ_USB );

  // return success
  return HCD_RESPONSE_OK;
}
