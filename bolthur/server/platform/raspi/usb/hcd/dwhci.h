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

#ifndef _DWHCI_H
#define _DWHCI_H

#include "../../../../libusb.h"
#include "../../../../libhcd.h"
#include "response.h"

// #define DWHCI_ENABLE_DEBUG 1

typedef enum {
  DWHCI_CHANNEL_STATE_DATA0 = 0,
  DWHCI_CHANNEL_STATE_DATA1 = 2,
  DWHCI_CHANNEL_STATE_DATA2 = 1,
  DWHCI_CHANNEL_STATE_MDATA = 3,
  DWHCI_CHANNEL_STATE_SETUP = 3,
} dwhci_channel_state_t;

typedef enum {
  DWHCI_QUEUE_CHANNEL_STATUS_PENDING = 0,
  DWHCI_QUEUE_CHANNEL_STATUS_SETUP = 1,
  DWHCI_QUEUE_CHANNEL_STATUS_DATA = 2,
  DWHCI_QUEUE_CHANNEL_STATUS_ACK = 3,
  DWHCI_QUEUE_CHANNEL_STATUS_DONE = 4,

  DWHCI_QUEUE_POLL_STATUS_PENDING = 5,
  DWHCI_QUEUE_POLL_STATUS_DATA = 6,
  DWHCI_QUEUE_POLL_STATUS_ACK = 7,
  DWHCI_QUEUE_POLL_STATUS_DONE = 8,
} dwhci_queue_status_t;

/**
 * @brief Channel queue entry
 */
typedef struct channel_queue_entry {
  /** queue entry data */
  void* data;
  /** channel that was used by queue entry */
  uint8_t channel;
  /** queue status */
  dwhci_queue_status_t status;
  /** data buffer */
  void* buffer;
  /** transferred data */
  uint32_t transferred;
  /** transferred packet count */
  uint32_t packet_transferred;
  /** response info */
  size_t response_info;
  /** message */
  void* message;
  /** error */
  libusb_transfer_error_t error;
  /** pointer to next entry */
  struct channel_queue_entry* next;
  /** pointer to previous entry */
  struct channel_queue_entry* prev;
} channel_queue_entry_t;

/**
 * @brief Configuration object
 */
typedef struct {
  /**
   * @brief Channel object
   */
  struct {
    /** channel count */
    uint32_t count;
    /** allocated channels */
    uint32_t allocated;
  } channel;
  /** queue entry list */
  channel_queue_entry_t* list;
} dwhci_configuration_t;

extern int fd_iomem;
extern void* databuffer;
extern dwhci_configuration_t configuration;

response_t dwhci_channel_interrupt_to_error( libusb_transfer_error_t*, uint8_t, bool );
response_t dwhci_transmit_channel( uint8_t, void* );
response_t dwhci_prepare_channel( uint32_t, uint32_t, uint8_t, uint32_t, dwhci_channel_state_t, const libusb_pipe_address_t* );
response_t dwhci_allocate_channel( uint8_t* );
response_t dwhci_free_channel( uint8_t );
response_t dwhci_queue_add_entry( void*, dwhci_queue_status_t, channel_queue_entry_t** );
response_t dwhci_queue_remove_entry( channel_queue_entry_t* );
response_t dwhci_queue_get_active_by_channel( uint8_t, channel_queue_entry_t** );
response_t dwhci_enable_channel_interrupt( uint8_t );
response_t dwhci_disable_channel_interrupt( uint8_t );
response_t dwhci_channel_send_async_start_channel( const channel_queue_entry_t* );
response_t dwhci_channel_send_async_stop_channel( const channel_queue_entry_t* );
response_t dwhci_channel_send_async_setup( channel_queue_entry_t* );
response_t dwhci_channel_send_async_data( channel_queue_entry_t* );
response_t dwhci_channel_send_async_ack( channel_queue_entry_t* );
response_t dwhci_channel_send_async_done( channel_queue_entry_t* );
response_t dwhci_channel_send_async_continue_pending( channel_queue_entry_t* );
response_t dwhci_channel_send_async_continue( channel_queue_entry_t* );
response_t dwhci_channel_send_async( hcd_control_message_t*, hcd_submit_control_message_t*, size_t );
response_t dwhci_channel_poll_async_data( channel_queue_entry_t* );
response_t dwhci_channel_poll_async_ack( channel_queue_entry_t* );
response_t dwhci_channel_poll_async_done( channel_queue_entry_t* );
response_t dwhci_channel_poll_async( hcd_interrupt_poll_t*, hcd_submit_interrupt_poll_t*, size_t );
response_t dwhci_next_usb_pid( dwhci_channel_state_t, uint32_t, uint8_t* );
response_t dwhci_read_port( uint32_t, uint32_t* );
response_t dwhci_write_port( uint32_t, uint32_t );
response_t dwhci_power_on( void );
response_t dwhci_core_flush_tx_fifo( uint32_t );
response_t dwhci_core_flush_rx_fifo( void );
response_t dwhci_init( void );

#endif
