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

#include <inttypes.h>
#include "../../mmio.h"
#include "../../rpc.h"
#include "../../constants.h"
#include "../../dwhci.h"
#include "../../../../libhcd.h"

/**
 * @fn void toggle_split_phase(const uint32_t, channel_queue_entry_t*);
 * @brief Wrapper to toggle between split phases
 * @param cipt
 * @param entry
 */
static bool toggle_split_phase( const uint32_t cipt, channel_queue_entry_t* entry ) {
  if ( DWHCI_SPLIT_PHASE_NONE == entry->split_phase ) {
    return false;
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
  // read interrupt register
  uint32_t interrupt = mmio_read( PERIPHERAL_DWHCI_CORE_INT_STAT );
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
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
    for ( uint32_t channel = 0; channel < configuration.channel.count; channel++ ) {
      // handle channel interrupt
      if ( channel_interrupt & channel_mask ) {
        // reset channel interrupts
        mmio_write( PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( channel ), 0 );
      }
      // assign channel
      channel_mask <<= 1;
    }
  }
  // fire handle done
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Mark interrupts as handled\r\n" )
  #endif
  _syscall_interrupt_handled();
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
    for ( uint32_t channel = 0; channel < configuration.channel.count; channel++ ) {
      // handle channel interrupt
      if ( channel_interrupt & channel_mask ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "channel = %"PRIu32"\r\n", channel )
        #endif
        // get queue entry matching to channel
        channel_queue_entry_t* entry;
        const response_t result = dwhci_queue_get_active_by_channel( ( uint8_t )channel, &entry );
        if ( HCD_RESPONSE_OK != result ) {
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "No queued entry found for channel %"PRIu32"\r\n", channel )
          #endif
          continue;
        }
        // get channel interrupt
        const uint32_t cipt = mmio_read( PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ) );
        //#if defined( DWHCI_ENABLE_DEBUG )
          if ( DWHCI_QUEUE_POLL_STATUS_DATA == entry->status )
          EARLY_STARTUP_PRINT( "cipt = %#"PRIx32"\r\n", cipt )
        //#endif
        if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE ) {
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "Transfer complete for channel %"PRIu32"\r\n", channel )
          #endif
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_HALT ) {
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "Halt for channel %"PRIu32"\r\n", channel )
          #endif
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
            EARLY_STARTUP_PRINT( "Nack for channel %"PRIu32"\r\n", channel )
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
          #endif
          entry->error |= LIBUSB_TRANSFER_ERROR_NOT_YET_ERROR;
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ) {
          #if defined( DWHCI_ENABLE_DEBUG )
            EARLY_STARTUP_PRINT( "Transaction error for channel %"PRIu32"\r\n", channel )
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
        const bool transfer_complete =
          cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE;

        // on short response we've to reset packet to transfer because it marks
        // the end of usb transaction
        if ( short_response ) {
          entry->packets_to_transfer = 0;
        }

        // handle split complete to overwrite transferred in case it's not a
        // short response
        if ( split_complete && ! short_response ) {
          transferred = requested;
        }
        // reduce packet size if something was transferred
        if (
          transferred > 0
          || ( transferred == 0 && split_complete )
          || (
            entry->split_phase == DWHCI_SPLIT_PHASE_NONE
            && entry->buffer_size_to_transfer == 0
          )
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
        if (
          ( cipt & HCD_CHANNEL_INTERRUPT_HALT )
          || ( cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE )
        ) {
          // handle finished
          if (
            // treat setup status with transfer complete as done
            (
              entry->status == DWHCI_QUEUE_CHANNEL_STATUS_SETUP
              && transfer_complete
            )
            // treat ack status with transfer complete as done
            || (
              entry->status == DWHCI_QUEUE_CHANNEL_STATUS_ACK
              && transfer_complete
            )
            || (
              entry->status == DWHCI_QUEUE_POLL_STATUS_DATA
              && (
                transfer_complete
                || ( cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT )
              )
            // treat cancellation as finished
            ) || entry->status == DWHCI_QUEUE_CANCEL
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
            //#if defined( DWHCI_ENABLE_DEBUG )
              if (DWHCI_QUEUE_POLL_STATUS_DATA == entry->status)
              EARLY_STARTUP_PRINT( "Restart current state with remaining, %"PRIu32" / %"PRIu32", transfer_size: %"PRIx32"\r\n",
                remaining, entry->buffer_size_to_transfer, transfer_size )
            //#endif
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
              && DWHCI_SPLIT_PHASE_NONE == entry->packets_to_transfer
            ) || (
              entry->buffer_size_to_transfer > 0
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
        // handle switch to next
        if ( switch_to_next_state ) {
          // evaluate next state
          switch ( entry->status ) {
            case DWHCI_QUEUE_CHANNEL_STATUS_SETUP:
              entry->packets_to_transfer = 0;
              entry->packet_size = 0;
              entry->status = DWHCI_QUEUE_CHANNEL_STATUS_DATA;
              break;
            case DWHCI_QUEUE_CHANNEL_STATUS_DATA:
              entry->packets_to_transfer = 0;
              entry->packet_size = 0;
              entry->status = DWHCI_QUEUE_CHANNEL_STATUS_ACK;
              break;
            case DWHCI_QUEUE_CHANNEL_STATUS_ACK:
              entry->status = DWHCI_QUEUE_CHANNEL_STATUS_DONE;
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
            default:
              #if defined( DWHCI_ENABLE_DEBUG )
                EARLY_STARTUP_PRINT( "Unknown status request for channel %"PRIu32"\r\n", channel )
              #endif
              continue;
          }
        }
        // continue with new step
        dwhci_channel_async_continue( entry );
      }
      // assign channel
      channel_mask <<= 1;
    }
  }
}
