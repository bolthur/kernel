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

#include "../../rpc.h"
#include "../../dwhci.h"
#include "../../../../libhcd.h"
#include "../../../../libperipheral.h"

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
  EARLY_STARTUP_PRINT( "Interrupt handler called\r\n" )
  // read interrupt register
  uint32_t interrupt;
  response_t result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_INT_STAT, &interrupt );
  if ( HCD_RESPONSE_OK != result ) {
    EARLY_STARTUP_PRINT( "Unable to read interrupt status register!\r\n" )
    return;
  }
  EARLY_STARTUP_PRINT( "interrupt = %#"PRIx32"\r\n", interrupt )
  // handle interrupt
  if ( interrupt & HCD_DWHCI_CORE_INT_MASK_HC_INTR ) {
    // read channel interrupts
    uint32_t channel_interrupt;
    result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_ALLCHAN_INT, &channel_interrupt );
    if ( HCD_RESPONSE_OK != result ) {
      EARLY_STARTUP_PRINT( "Unable to read all channel interrupt status register!\r\n" )
      return;
    }
    EARLY_STARTUP_PRINT( "channel_interrupt = %#"PRIx32"\r\n", channel_interrupt )
    // write back value
    result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_ALLCHAN_INT, channel_interrupt );
    if ( HCD_RESPONSE_OK != result ) {
      EARLY_STARTUP_PRINT( "Unable to write back all channel interrupt status register!\r\n" )
      return;
    }
    // iterate over channels
    uint32_t channel_mask = 1;
    for ( uint32_t channel = 0; channel < configuration.channel.count; channel++ ) {
      // handle channel interrupt
      if ( channel_interrupt & channel_mask ) {
        // get queue entry matching to channel
        channel_queue_entry_t* entry;
        result = dwhci_queue_get_active_by_channel( ( uint8_t )channel, &entry );
        if ( HCD_RESPONSE_OK != result ) {
          EARLY_STARTUP_PRINT( "No queued entry found for channel %"PRIu32"\r\n", channel )
          continue;
        }
        // get channel interrupt
        uint32_t cipt;
        result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ), &cipt );
        if ( HCD_RESPONSE_OK != result ) {
          EARLY_STARTUP_PRINT( "Unable to read channel interrupt status register!\r\n" )
          continue;
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE ) {
          EARLY_STARTUP_PRINT( "Transfer complete for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_HALT ) {
          EARLY_STARTUP_PRINT( "Halt for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_AHB_ERROR ) {
          EARLY_STARTUP_PRINT( "AHB Error for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_STALL ) {
          EARLY_STARTUP_PRINT( "Stall for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT ) {
          EARLY_STARTUP_PRINT( "Nack for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT ) {
          EARLY_STARTUP_PRINT( "Ack for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_NOT_YET ) {
          EARLY_STARTUP_PRINT( "Not yet for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ) {
          EARLY_STARTUP_PRINT( "Transaction error for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_BABBLE_ERROR ) {
          EARLY_STARTUP_PRINT( "Babble error for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_FRAME_OVERRUN ) {
          EARLY_STARTUP_PRINT( "Frame overrun for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_DATA_TOGGLE_ERROR ) {
          EARLY_STARTUP_PRINT( "Data toggle error for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_BUFFER_NOT_AVAILABLE ) {
          EARLY_STARTUP_PRINT( "Buffer not available for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_EXCESSIVE_TRANSMISSION ) {
          EARLY_STARTUP_PRINT( "Excessive transmission for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_FRAME_LIST_ROLLOVER ) {
          EARLY_STARTUP_PRINT( "Rollover for channel %"PRIu32"\r\n", channel )
        }
        if ( cipt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE ) {
          uint32_t transfer_size;
          result = dwhci_read_port(
            ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ),
            &transfer_size
          );
          if ( HCD_RESPONSE_OK != result ) {
            EARLY_STARTUP_PRINT( "Unable to read transfer size register!\r\n" )
            continue;
          }
          entry->transferred = HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_TRANSFER_SIZE( transfer_size );
        }
        // evaluate next state
        switch ( entry->status ) {
          case DWHCI_QUEUE_STATUS_SETUP:
            entry->status = DWHCI_QUEUE_STATUS_DATA;
            break;
          case DWHCI_QUEUE_STATUS_DATA:
            entry->status = DWHCI_QUEUE_STATUS_ACK;
            break;
          case DWHCI_QUEUE_STATUS_ACK:
            entry->status = DWHCI_QUEUE_STATUS_DONE;
            break;
          default:
            EARLY_STARTUP_PRINT( "Unknown status request for channel %"PRIu32"\r\n", channel )
            continue;
        }
        // continue with new step
        dwhci_channel_send_async_continue( entry );
      }
      // assign channel
      channel_mask <<= 1;
    }
  }
  // mask all pending interrupts
  result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_INT_STAT, interrupt );
  if ( HCD_RESPONSE_OK != result ) {
    EARLY_STARTUP_PRINT( "Unable to write interrupt status register!\r\n" )
    return;
  }
  // acquire interrupt again
  _syscall_interrupt_acquire( ARM_IRQ_USB );
}
