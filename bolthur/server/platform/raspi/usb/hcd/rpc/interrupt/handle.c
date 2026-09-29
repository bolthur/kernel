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

#include <assert.h>
#include <inttypes.h>
#include "../../mmio.h"
#include "../../rpc.h"
#include "../../constants.h"
#include "../../dwhci.h"
#include "../../../../libhcd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void toggle_split_phase(const uint32_t, channel_queue_entry_t*);
 * @brief Wrapper to toggle between split phases
 * @param cipt
 * @param entry
 */
static bool toggle_split_phase( const uint32_t cipt, channel_queue_entry_t* entry ) {
  if ( DWHCI_SPLIT_PHASE_NONE == entry->split_phase ) {
    return true;
  }
  if ( DWHCI_SPLIT_PHASE_SSPLIT == entry->split_phase ) {
    if (
      cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT
      && DWHCI_QUEUE_POLL_STATUS_DATA != entry->status
    ) {
      return false;
    }
    if ( cipt & HCD_CHANNEL_INTERRUPT_NOT_YET ) {
      return false;
    }
    entry->split_phase = DWHCI_SPLIT_PHASE_CSPLIT;
    return false;
  } else if ( DWHCI_SPLIT_PHASE_CSPLIT == entry->split_phase ) {
    if ( cipt & HCD_CHANNEL_INTERRUPT_NOT_YET ) {
      return false;
    }
    if (
      cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT
      && DWHCI_QUEUE_POLL_STATUS_DATA != entry->status
    ) {
      return false;
    }
    if (
      cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT
      && DWHCI_QUEUE_POLL_STATUS_DATA == entry->status
    ) {
      entry->split_phase = DWHCI_SPLIT_PHASE_SSPLIT;
      return true;
    }
    if (cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE) {
      // CSPLIT completed the current USB transaction.
      entry->split_phase = DWHCI_SPLIT_PHASE_SSPLIT;
      return true;
    }

  }
  return false;
}

/**
 * @fn void wait_for_next_microframe( void )
 * @brief Helper to wait for next microframe
 * @param micro_frames_to_wait
 */
static void wait_for_next_microframe( uint32_t micro_frames_to_wait ) {
  if ( 0 == micro_frames_to_wait ) {
    return;
  }
  uint32_t start_frame = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM ) & 0xFFFF;
  uint32_t current_frame;
  uint32_t elapsed_micro_frames;
  do {
    current_frame = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM ) & 0xFFFF;
    if ( current_frame >= start_frame ) {
      elapsed_micro_frames = current_frame - start_frame;
    } else {
      elapsed_micro_frames = ( ( uint16_t )-1 - start_frame ) + current_frame;
    }
    __asm__ __volatile__ ( "nop" ::: "memory" );
  } while ( elapsed_micro_frames < micro_frames_to_wait );
}

/**
 * @fn void rpc_interrupt_handle(size_t, pid_t, size_t, size_t)
 * @brief Interrupt handler
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_interrupt_handle(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Interrupt handler called\r\n" )
  #endif
  [[maybe_unused]] uint32_t frame_num_entry = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
  // read interrupt register
  const uint32_t interrupt = mmio_read( PERIPHERAL_DWHCI_CORE_INT_STAT );
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
    if ( ! interrupt ) {
      EARLY_STARTUP_PRINT( "interrupt is 0\r\n" )
    }
  #endif
  // mask pending interrupts
  mmio_write( PERIPHERAL_DWHCI_CORE_INT_STAT, interrupt );
  uint32_t channel_interrupt = 0;
  uint32_t channel_mask = 1;
  if ( interrupt & HCD_DWHCI_CORE_INT_MASK_HC_INTR ) {
    // read channel interrupts
    channel_interrupt = mmio_read( PERIPHERAL_DWHCI_HOST_ALLCHAN_INT );
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "channel_interrupt = %#"PRIx32"\r\n", channel_interrupt )
    #endif
    // mask channel interrupts
    mmio_write( PERIPHERAL_DWHCI_HOST_ALLCHAN_INT, channel_interrupt );
    // iterate over channels
    for ( uint32_t channel = 0; channel < configuration.channel.count; channel++, channel_mask <<= 1 ) {
      // handle channel interrupt
      if ( channel_interrupt & channel_mask ) {
        // reset channel interrupts
        mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( channel ), 0 );
      }
    }
  }
  // acquire interrupt again
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Acquiring interrupt again\r\n" )
  #endif
  _syscall_interrupt_acquire( ARM_IRQ_USB );
  // handle interrupt itself
  if ( interrupt & HCD_DWHCI_CORE_INT_MASK_HC_INTR ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handling now channel interrupts ( interruptable sequence )\r\n" )
    #endif
    // iterate over channels
    channel_mask = 1;
    for ( uint32_t channel = 0; channel < configuration.channel.count; channel++, channel_mask <<= 1 ) {
      // handle no channel interrupt
      if ( ! ( channel_interrupt & channel_mask ) ) {
        continue;
      }
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "channel = %"PRIu32"\r\n", channel )
      #endif
      // get queue entry matching to channel
      channel_queue_entry_t* entry;
      const response_t result = dwhci_queue_get_active_by_channel( ( uint8_t )channel, &entry );
      if ( HCD_RESPONSE_OK != result ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "No queued entry found for channel %"PRIu32"\r\n", channel )
        #endif
        // skip rest
        continue;
      }
      // get channel interrupt
      const uint32_t cipt = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ) );
      // write back to mark them as handled
      mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ), cipt );

      #if defined( DWHCI_ENABLE_DEBUG )
        libusb_transfer_error_t previous = entry->previous_transfer_status;
      #endif

      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
        EARLY_STARTUP_PRINT( "status = %d\r\n", entry->status )
      #endif
      if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Transfer complete for channel %"PRIu32"\r\n", channel )
        #endif
        entry->transfer_status |= LIBUSB_TRANSFER_ERROR_COMPLETE;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_HALT ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Halt for channel %"PRIu32"\r\n", channel )
        #endif
        entry->transfer_status |= LIBUSB_TRANSFER_ERROR_HALT;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_AHB_ERROR ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "AHB Error for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_AHB_ERROR;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_STALL ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Stall for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_STALL;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "status = %d\r\n", entry->status )
          EARLY_STARTUP_PRINT( "Nack for channel %"PRIu32"\r\n", channel )
          EARLY_STARTUP_PRINT( "previous transfer = %#x\r\n", previous )
          EARLY_STARTUP_PRINT( "transfer = %#x\r\n", entry->transfer_status )
          EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
          EARLY_STARTUP_PRINT(
            "HCCHAR=%#"PRIx32" HCSPLT=%#"PRIx32" HCTSIZ=%#"PRIx32" HFNUM=%#"PRIx32"\r\n",
            entry->verify_char,
            entry->verify_split,
            entry->verify_size,
            entry->verify_num
          )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Ack for channel %"PRIu32"\r\n", channel )
        #endif
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_NOT_YET ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Not yet for channel %"PRIu32"\r\n", channel )
          EARLY_STARTUP_PRINT( "status = %d\r\n", entry->status )
          EARLY_STARTUP_PRINT( "previous transfer = %#x\r\n", previous )
          EARLY_STARTUP_PRINT( "transfer = %#x\r\n", entry->transfer_status )
          EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
          EARLY_STARTUP_PRINT(
            "HCCHAR=%#"PRIx32" HCSPLT=%#"PRIx32" HCTSIZ=%#"PRIx32" HFNUM=%#"PRIx32"\r\n",
            entry->verify_char,
            entry->verify_split,
            entry->verify_size,
            entry->verify_num
          )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_NOT_YET_ERROR;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Transaction error for channel %"PRIu32"\r\n", channel )
          EARLY_STARTUP_PRINT( "status = %d\r\n", entry->status )
          EARLY_STARTUP_PRINT( "Nack for channel %"PRIu32"\r\n", channel )
          EARLY_STARTUP_PRINT( "previous transfer = %#x\r\n", previous )
          EARLY_STARTUP_PRINT( "transfer = %#x\r\n", entry->transfer_status )
          EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
          EARLY_STARTUP_PRINT(
            "HCCHAR=%#"PRIx32" HCSPLT=%#"PRIx32" HCTSIZ=%#"PRIx32" HFNUM=%#"PRIx32"\r\n",
            entry->verify_char,
            entry->verify_split,
            entry->verify_size,
            entry->verify_num
          )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_TRANSACTION;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_BABBLE_ERROR ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Babble error for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_BABBLE;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_FRAME_OVERRUN ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Frame overrun for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_FRAME_OVERRUN;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_DATA_TOGGLE_ERROR ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Data toggle error for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_DATA_TOGGLE;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_BUFFER_NOT_AVAILABLE ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Buffer not available for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_BUFFER_NOT_AVAILABLE;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_EXCESSIVE_TRANSMISSION ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Excessive transmission for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_BUFFER_EXCESSIVE_TRANSMISSION;
      }
      if ( cipt & HCD_CHANNEL_INTERRUPT_FRAME_LIST_ROLLOVER ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Rollover for channel %"PRIu32"\r\n", channel )
        #endif
        entry->error |= LIBUSB_TRANSFER_ERROR_LIST_ROLLOVER;
      }
      // extract transfer size
      const uint32_t transfer_size = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ) );
      // extract remaining
      const uint32_t remaining = HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( transfer_size );
      // calculate transferred
      uint32_t transferred = entry->buffer_size_to_transfer - remaining;

      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "transfer_size = %"PRIx32"\r\n", transfer_size )
      #endif

      const uint32_t requested = entry->packet_size < entry->buffer_size_to_transfer
        ? entry->packet_size : entry->buffer_size_to_transfer;
      // toggle possible split phase entry
      const bool split_complete = toggle_split_phase( cipt, entry );
      // handle short transfer
      const bool short_response = transferred != 0 && transferred < requested;
      // transfer complete flag
      const bool transfer_complete = cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE;
      const bool channel_halted = cipt & HCD_CHANNEL_INTERRUPT_HALT;
      const bool channel_nack = cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT;
      const bool channel_not_yet = cipt & HCD_CHANNEL_INTERRUPT_NOT_YET;

      // handle nack and not yet => immediate retry
      if (
        // treat nack as retry
        (
          channel_nack
          && DWHCI_QUEUE_POLL_STATUS_DATA != entry->status
        // treat not yet with no split as retry
        ) || (
          channel_not_yet
          && DWHCI_QUEUE_POLL_STATUS_DATA != entry->status
        // treat split phase with no split complete as retry
        )/* || (
          ! transfer_complete
          && ! split_complete
          && DWHCI_QUEUE_POLL_STATUS_DATA != entry->status
        )*/
      ) {
        // reset error
        entry->error = 0;
        // handle possible wait for next microframe
        entry->csplit_frame_num = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
        const uint32_t ssplit_frame = ( entry->ssplit_frame_num >> 3 ) & 0x7FF;
        const uint32_t ssplit_uframe = entry->ssplit_frame_num & 0x7;
        const uint32_t csplit_frame = ( entry->csplit_frame_num >> 3 ) & 0x7FF;
        const uint32_t csplit_uframe = entry->csplit_frame_num & 0x7;
        // calculate passed frames
        const uint32_t start = entry->ssplit_frame_num & 0xFFFF;
        const uint32_t current = entry->csplit_frame_num & 0xFFFF;
        const uint32_t passed_frames = current >= start ? current - start : ( ( uint16_t )-1 - start ) + current;
        // wait for next micro frame if it's below 2
        if ( ssplit_frame == csplit_frame && ssplit_uframe == csplit_uframe ) {
          wait_for_next_microframe( 1 );
        }
        if ( entry->csplit_frame_num_previous != 0 && DWHCI_SPLIT_PHASE_NONE != entry->split_phase ) {
          const uint32_t csplit_frame_previous = ( entry->csplit_frame_num_previous >> 3 ) & 0x7FF;
          const uint32_t csplit_uframe_previous = entry->csplit_frame_num_previous & 0x7;
          // wait for next micro frame if it's below 2
          if ( csplit_frame_previous == csplit_frame && csplit_uframe_previous == csplit_uframe ) {
            wait_for_next_microframe( 1 );
          }
        }
        // calculate difference and finally passed milliseconds
        const uint64_t tick = _syscall_timer_tick_count();
        const uint64_t difference = tick - entry->last_tick_count;
        const uint64_t passed_milliseconds = ( uint64_t )( ( ( double )difference / ( double )entry->timer_frequency ) * 1000.0 );
        // handle smaller
        const bool split_transaction_timeout_reached = passed_milliseconds >= entry->setup_timeout;
        // handle frame miss
        if ( split_transaction_timeout_reached ) {
          EARLY_STARTUP_PRINT( "tick = %"PRIu64"\r\n", tick )
          EARLY_STARTUP_PRINT( "last_tick_count = %"PRIu64"\r\n", entry->last_tick_count )
          EARLY_STARTUP_PRINT( "passed_milliseconds = %"PRIu64"\r\n", passed_milliseconds )
          EARLY_STARTUP_PRINT( "setup_timeout = %zu\r\n", entry->setup_timeout )
          EARLY_STARTUP_PRINT( "frame window missed: %"PRIu32"\r\n", passed_frames / 8 )
          entry->error |= LIBUSB_TRANSFER_ERROR_EAGAIN;
          EARLY_STARTUP_PRINT( "EAGAIN\r\n" )
          EARLY_STARTUP_PRINT(
            "SSPLIT: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
            entry->ssplit_frame_num,
            (entry->ssplit_frame_num >> 3) & 0x7FF,
            entry->ssplit_frame_num & 0x7
          )
          EARLY_STARTUP_PRINT(
            "RPC ENTRY: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
            frame_num_entry,
            (frame_num_entry >> 3) & 0x7FF,
            frame_num_entry & 0x7
          )
          EARLY_STARTUP_PRINT(
            "CSPLIT: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
            entry->csplit_frame_num,
            (entry->csplit_frame_num >> 3) & 0x7FF,
            entry->csplit_frame_num & 0x7
          )
        } else {
          if ( DWHCI_SPLIT_PHASE_CSPLIT == entry->split_phase ) {
            // read out split ctrl, set complete split and write it back
            uint32_t split_control = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ) );
            split_control |= HCD_DWHCI_CHAN_SPLIT_CONTROL_COMPLETE_SPLIT( 1 );
            mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ), split_control );
          }
          dwhci_channel_state_t packet_id = DWHCI_CHANNEL_STATE_SETUP;
          if ( DWHCI_QUEUE_CHANNEL_STATUS_DATA == entry->status ) {
            packet_id = entry->channel_data_state;
          } else if ( DWHCI_QUEUE_CHANNEL_STATUS_ACK == entry->status ) {
            packet_id = DWHCI_CHANNEL_STATE_DATA1;
          }
          // set transfer size, packet id and packet count again
          const uint32_t transfer_data = HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( entry->buffer_size_to_transfer )
            | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_ID( packet_id )
            | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_COUNT( entry->transaction_packet_count );
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ), transfer_data );
          // write int mask again
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( channel ),
            HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE
            | HCD_CHANNEL_INTERRUPT_HALT
            | HCD_CHANNEL_INTERRUPT_ERROR_MASK
            | HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT
            | HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT
            | HCD_CHANNEL_INTERRUPT_NOT_YET
          );
          // write transfer size
          entry->csplit_frame_num_previous = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
          // read character and enable it again
          uint32_t characteristic = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ) );
          characteristic &= ~HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 );
          characteristic |= HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ), characteristic );
          // debug output
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT(
              "SSPLIT: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
              entry->ssplit_frame_num,
              (entry->ssplit_frame_num >> 3) & 0x7FF,
              entry->ssplit_frame_num & 0x7
            )
            EARLY_STARTUP_PRINT(
              "RPC ENTRY: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
              frame_num_entry,
              (frame_num_entry >> 3) & 0x7FF,
              frame_num_entry & 0x7
            )
            EARLY_STARTUP_PRINT(
              "CSPLIT: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
              entry->csplit_frame_num,
              (entry->csplit_frame_num >> 3) & 0x7FF,
              entry->csplit_frame_num & 0x7
            )
          #endif
          // skip rest
          continue;
        }
      }

      // on short response we've to reset packet to transfer because it marks
      // the end of usb transaction
      if ( short_response ) {
        entry->packets_to_transfer = 0;
      }

      bool split_transaction_timeout_reached = false;
      // handle csplit for interrupt polling
      if (
        DWHCI_SPLIT_PHASE_CSPLIT == entry->split_phase
        && DWHCI_QUEUE_POLL_STATUS_DATA == entry->status
        && ! split_complete
        && ! transfer_complete
        && ! channel_nack
      ) {
        // reset error
        entry->error = 0;
        // calculate difference and finally passed milliseconds
        const uint64_t tick = _syscall_timer_tick_count();
        const uint64_t difference = tick - entry->last_tick_count;
        const uint64_t passed_milliseconds = ( uint64_t )( ( ( double )difference / ( double )entry->timer_frequency ) * 1000.0 );
        // handle smaller
        split_transaction_timeout_reached = passed_milliseconds >= entry->poll_timeout;
        if ( ! split_transaction_timeout_reached ) {
          // read out split ctrl, set complete split and write it back
          uint32_t split_control = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ) );
          split_control |= HCD_DWHCI_CHAN_SPLIT_CONTROL_COMPLETE_SPLIT( 1 );
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ), split_control );
          // evaluate paket count
          const usb_interrupt_poll_t* entry_data = entry->data;
          uint32_t packet_count = ( entry->buffer_size_to_transfer + 7 ) / 8;
          if ( LIBUSB_SPEED_LOW != entry_data->pipe_address.speed ) {
            packet_count = ( entry->buffer_size_to_transfer + usb_number_from_packet_size( entry_data->pipe_address.max_size ) - 1 ) / usb_number_from_packet_size( entry_data->pipe_address.max_size );
          }
          if ( 0 == packet_count ) {
            packet_count = 1;
          }
          // write transfer data again
          const uint32_t transfer_data = HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( entry->buffer_size_to_transfer )
            | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_ID( entry->poll_state )
            | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_COUNT( packet_count );
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ), transfer_data );
          // write int mask again
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( channel ),
            HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE
            | HCD_CHANNEL_INTERRUPT_HALT
            | HCD_CHANNEL_INTERRUPT_ERROR_MASK
            | HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT
            | HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT
            | HCD_CHANNEL_INTERRUPT_NOT_YET
          );
          // handle possible wait for next microframe
          entry->csplit_frame_num = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
          const uint32_t ssplit_frame = ( entry->ssplit_frame_num >> 3 ) & 0x7FF;
          const uint32_t ssplit_uframe = entry->ssplit_frame_num & 0x7;
          const uint32_t csplit_frame = ( entry->csplit_frame_num >> 3 ) & 0x7FF;
          const uint32_t csplit_uframe = entry->csplit_frame_num & 0x7;
          if ( ssplit_frame == csplit_frame && ssplit_uframe == csplit_uframe ) {
            wait_for_next_microframe( 1 );
          }
          // calculate target frame
          const uint32_t frame_number = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
          const uint32_t current_frame = ( frame_number >> 3 ) & 0x7FF;
          const uint32_t current_uframe = frame_number & 0x7;
          const uint32_t current_linear_uframe = current_frame * 8 + current_uframe;
          const uint32_t target_linear_uframe = current_linear_uframe + 1;
          const uint32_t target_frame = ( target_linear_uframe / 8 ) & 0x7FF;
          // read out host chan character, ensure that disable bit is not set, set enable
          uint32_t characteristic = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ) );
          characteristic &= ~HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 );
          characteristic &= ~HCD_DWHCI_CHAN_CHARACTER_ODD_FRAME( 1 );
          characteristic |= HCD_DWHCI_CHAN_CHARACTER_ODD_FRAME( target_frame & 1 );
          characteristic |= HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
          entry->csplit_frame_num = mmio_read( PERIPHERAL_DWHCI_HOST_FRM_NUM );
          // write back character
          mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ), characteristic );
          // debug output
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT(
              "SSPLIT: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
              entry->ssplit_frame_num,
              (entry->ssplit_frame_num >> 3) & 0x7FF,
              entry->ssplit_frame_num & 0x7
            )
            EARLY_STARTUP_PRINT(
              "RPC ENTRY: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
              frame_num_entry,
              (frame_num_entry >> 3) & 0x7FF,
              frame_num_entry & 0x7
            )
            EARLY_STARTUP_PRINT(
              "CSPLIT: HFNUM = %#"PRIx32" frame=%"PRIu32" uframe=%"PRIu32"\r\n",
              entry->csplit_frame_num,
              (entry->csplit_frame_num >> 3) & 0x7FF,
              entry->csplit_frame_num & 0x7
            )
            // print interrupt
            EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
            EARLY_STARTUP_PRINT( "split_control = %#"PRIx32"\r\n", split_control )
            EARLY_STARTUP_PRINT( "transfer_data = %#"PRIx32"\r\n", transfer_data )
            EARLY_STARTUP_PRINT( "characteristic = %#"PRIx32"\r\n", characteristic )
            EARLY_STARTUP_PRINT( "entry->channel = %"PRIu32"\r\n", channel )
          #endif
          // skip rest
          continue;
        }
        // continue with poll cancel
        entry->status = DWHCI_QUEUE_POLL_STATUS_CANCEL;
      }

      // debug output
      #if defined( DWCHI_ENABLE_DEBUG )
        if ( DWHCI_QUEUE_POLL_STATUS_DATA == entry->status ) {
          EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
        }
      #endif

      // handle split complete to overwrite transferred in case it's not a
      // short response
      if ( split_complete && ! short_response ) {
        transferred = requested;
      }
      // reduce packet size if something was transferred
      if (
        transferred > 0
        || ( transferred == 0 && split_complete )
      ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "packets to transfer:%"PRIu32", transferred = %"PRIu32", short_response: %d\r\n",
            entry->packets_to_transfer, transferred, short_response ? 1 : 0 )
        #endif
        // overwrite transferred when we've a transfer of 0 with
        // split complete and no short response
        if ( transferred == 0 && split_complete && ! short_response ) {
          transferred = entry->packet_size;
        }
        // in case it's not a short response, we've to reduce the packets to
        // transfer
        if ( ! short_response ) {
          // if there is some leftover at the end, we have a modulo result and
          // have to transfer packets manually
          if ( transferred % entry->packet_size ) {
            entry->packets_to_transfer--;
          } else {
            entry->packets_to_transfer -= ( transferred / entry->packet_size );
          }
        }
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "packets to transfer:%"PRIu32", transferred = %"PRIu32"\r\n",
            entry->packets_to_transfer, transferred )
        #endif
      }

      // set previous state to current state
      entry->previous_status = entry->status;

      // handle halt without transfer complete by checking for possible complete
      bool switch_to_next_state = false;
      // handle halt / complete
      if ( channel_halted || transfer_complete ) {
        // handle finished
        if (
          // treat setup status with transfer complete as done
          (
            DWHCI_QUEUE_CHANNEL_STATUS_SETUP == entry->status
            && transfer_complete
          )
          // treat ack status with transfer complete as done
          || (
            DWHCI_QUEUE_CHANNEL_STATUS_ACK == entry->status
            && transfer_complete
          )
          || (
            DWHCI_QUEUE_POLL_STATUS_DATA == entry->status
            && (
              transfer_complete
              || ( cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT )
            )
          // treat cancellation as finished
          ) || DWHCI_QUEUE_CANCEL == entry->status
          // treat poll cancellation as finished
          || (
            DWHCI_QUEUE_POLL_STATUS_CANCEL == entry->status
            && ! split_transaction_timeout_reached
          )
          // data / ack cancellation retry handling
          || DWHCI_QUEUE_CHANNEL_STATUS_DATA_CANCEL_RETRY == entry->status
          || DWHCI_QUEUE_CHANNEL_STATUS_ACK_CANCEL_RETRY == entry->status
          // treat no remaining as finished
          || entry->packets_to_transfer == 0
        ) {
          // debug output
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "Continue with next state\r\n" )
          #endif
          // toggle switch to next state
          switch_to_next_state = true;
          // set final transferred size
          if (
            entry->status == DWHCI_QUEUE_CHANNEL_STATUS_DATA
            || entry->status == DWHCI_QUEUE_CHANNEL_STATUS_ACK
            || entry->status == DWHCI_QUEUE_POLL_STATUS_DATA
          ) {
            entry->transferred = entry->buffer_offset + transferred;
          }
          // debug output
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "entry->transferred = %"PRIu32"\r\n", entry->transferred )
          #endif
          // reset buffer offset
          entry->buffer_offset = 0;
        } else {
          // debug output
          #if defined( DWHCI_ENABLE_DEBUG )
            if (DWHCI_QUEUE_POLL_STATUS_DATA == entry->status)
            EARLY_STARTUP_PRINT( "Restart current state with remaining, %"PRIu32" / %"PRIu32", transfer_size: %"PRIx32"\r\n",
              remaining, entry->buffer_size_to_transfer, transfer_size )
          #endif
          // increase buffer offset
          entry->buffer_offset += transferred;
        }
      }
      if (
        entry->status == DWHCI_QUEUE_CHANNEL_STATUS_DATA
        && ! switch_to_next_state
        && (
          (
            entry->packets_to_transfer > 0
            && DWHCI_SPLIT_PHASE_NONE == entry->split_phase
          ) || (
            entry->packets_to_transfer > 0
            && split_complete
          )
        )
      ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "entry->channel_data_state = %x\r\n", entry->channel_data_state )
        #endif
        entry->channel_data_state = DWHCI_CHANNEL_STATE_DATA0 == entry->channel_data_state
          ? DWHCI_CHANNEL_STATE_DATA1 : DWHCI_CHANNEL_STATE_DATA0;
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "entry->channel_data_state = %x\r\n", entry->channel_data_state )
        #endif
      }
      // toggle poll state
      if (
        DWHCI_QUEUE_POLL_STATUS_DATA == entry->status
        && transfer_complete
      ) {
        entry->poll_state = DWHCI_CHANNEL_STATE_DATA0 == entry->poll_state
          ? DWHCI_CHANNEL_STATE_DATA1 : DWHCI_CHANNEL_STATE_DATA0;
      }
      // handle error
      if ( ! switch_to_next_state && entry->error ) {
        EARLY_STARTUP_PRINT(
          "characteristic = %#"PRIx32", transfer_status = %#"PRIx32", split_control = %#"PRIx32"\r\n",
          mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ) ),
          mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ) ),
          mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ) ) )
        EARLY_STARTUP_PRINT(
          "entry->error = %#x, entry->transfer_status = %#x, entry->status = %d, entry->previous_status = %d\r\n",
          entry->error, entry->transfer_status, entry->status, entry->previous_status )
        if ( DWHCI_QUEUE_POLL_STATUS_DATA == entry->status ) {
          entry->status = DWHCI_QUEUE_POLL_STATUS_DONE;
        } else {
          entry->status = DWHCI_QUEUE_CHANNEL_STATUS_DONE;
        }
      }
      // handle switch to next
      if ( switch_to_next_state ) {
        // evaluate next state
        switch ( entry->status ) {
          case DWHCI_QUEUE_CHANNEL_STATUS_SETUP:
            entry->packets_to_transfer = 0;
            entry->packet_size = 0;
            const usb_control_message_t* entry_data = entry->data;
            // set next status depending on buffer length
            entry->status = entry_data->buffer_length
              ? DWHCI_QUEUE_CHANNEL_STATUS_DATA
              : DWHCI_QUEUE_CHANNEL_STATUS_ACK;
            break;
          case DWHCI_QUEUE_CHANNEL_STATUS_DATA:
            entry->packets_to_transfer = 0;
            entry->packet_size = 0;
            entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
            break;
          case DWHCI_QUEUE_CHANNEL_STATUS_DATA_CANCEL_RETRY:
            entry->status = DWHCI_QUEUE_CHANNEL_STATUS_DATA;
            break;
          case DWHCI_QUEUE_CHANNEL_STATUS_ACK:
            entry->status = DWHCI_QUEUE_CHANNEL_STATUS_DONE;
            break;
          case DWHCI_QUEUE_CHANNEL_STATUS_ACK_CANCEL_RETRY:
            entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
            break;
          case DWHCI_QUEUE_POLL_STATUS_DATA:
            entry->status = DWHCI_QUEUE_POLL_STATUS_ACK;
            break;
          case DWHCI_QUEUE_POLL_STATUS_ACK:
            entry->status = DWHCI_QUEUE_POLL_STATUS_DONE;
            break;
          case DWHCI_QUEUE_CANCEL:
            entry->status = DWHCI_QUEUE_CANCEL_DONE;
            break;
          case DWHCI_QUEUE_POLL_STATUS_CANCEL:
            // continue with ack
            entry->status = DWHCI_QUEUE_POLL_STATUS_ACK;
            // reset split phase
            if ( entry->split_phase != DWHCI_SPLIT_PHASE_NONE ) {
              entry->split_phase = DWHCI_SPLIT_PHASE_SSPLIT;
            }
            // fake a nack
            entry->error = LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE;
            break;
          default:
            #if defined( DWHCI_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Unknown status request for channel %"PRIu32"\r\n", channel )
            #endif
            // skip rest
            continue;
        }
      }
      // continue with new step
      dwhci_channel_async_continue( entry );
      // print cipt
      #if defined( DWHCI_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
      #endif
    }
  }
  // fire handle done
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Mark interrupts as handled\r\n" )
  #endif
  _syscall_interrupt_handled();
}
