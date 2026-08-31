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
#include "../../../../libusbd.h"
#include "response.h"

//#define DWHCI_ENABLE_DEBUG 1

typedef enum {
  DWHCI_CHANNEL_STATE_DATA0 = 0,
  DWHCI_CHANNEL_STATE_DATA1 = 2,
  DWHCI_CHANNEL_STATE_DATA2 = 1,
  DWHCI_CHANNEL_STATE_MDATA = 3,
  DWHCI_CHANNEL_STATE_SETUP = 3,
} dwhci_channel_state_t;

typedef enum {
  DWHCI_SPLIT_PHASE_NONE = 0,
  DWHCI_SPLIT_PHASE_SSPLIT = 1,
  DWHCI_SPLIT_PHASE_CSPLIT = 2,
} dwhci_split_phase_t;

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
  DWHCI_QUEUE_POLL_STATUS_WAIT = 9,

  DWHCI_QUEUE_CANCEL = 10,
  DWHCI_QUEUE_CANCEL_DONE = 11,
} dwhci_queue_status_t;

/**
 * @brief Channel queue entry
 */
typedef struct channel_queue_entry {
  /** queue entry data */
  void* data;
  /** data size */
  size_t data_size;
  /** channel that was used by queue entry */
  uint8_t channel;
  /** queue status */
  dwhci_queue_status_t status;
  /** queue status */
  dwhci_queue_status_t previous_status;
  /** split phase */
  dwhci_split_phase_t split_phase;
  /** data buffer */
  void* buffer;
  /** transfer buffer size */
  uint32_t buffer_size_to_transfer;
  /** buffer offset for transfer */
  uint32_t buffer_offset;
  /** transferred data */
  uint32_t transferred;
  /** response info */
  size_t response_info;
  /** origin */
  pid_t origin;
  /** channel prepared flag */
  bool prepared;
  /** interval */
  uint32_t interval;
  /** message */
  void* message;
  /** message size */
  size_t message_size;
  /** registered timer */
  size_t timer;
  /** error */
  libusb_transfer_error_t error;
  /** poll channel state */
  dwhci_channel_state_t poll_state;
  /** last poll timer */
  size_t poll_last_timer;
  /** poll timer */
  size_t poll_timer_id;
  /** channel data state */
  dwhci_channel_state_t channel_data_state;
  /** packets to transfer */
  uint32_t packets_to_transfer;
  /** packet size */
  uint32_t packet_size;
  uint32_t poll_ssplit_frame_num;
  uint32_t poll_csplit_frame_num;
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

response_t dwhci_prepare_channel( uint32_t, uint32_t, uint8_t, uint32_t, dwhci_channel_state_t, const libusb_pipe_address_t*, uint32_t, bool, channel_queue_entry_t* );
response_t dwhci_allocate_channel( uint8_t* );
response_t dwhci_free_channel( uint8_t );
response_t dwhci_queue_add_entry( void*, size_t, dwhci_queue_status_t, channel_queue_entry_t** );
void dwhci_queue_queue_entry( channel_queue_entry_t* );
response_t dwhci_queue_remove_entry( channel_queue_entry_t*, bool );
response_t dwhci_queue_get_active_by_channel( uint8_t, channel_queue_entry_t** );
response_t dwhci_enable_channel_interrupt( uint8_t );
response_t dwhci_disable_channel_interrupt( uint8_t );
response_t dwhci_channel_prepare_dma( channel_queue_entry_t* );
response_t dwhci_channel_send_async_start_channel( channel_queue_entry_t* );
response_t dwhci_channel_send_async_stop_channel( const channel_queue_entry_t*, bool );
response_t dwhci_channel_send_async_setup( channel_queue_entry_t* );
response_t dwhci_channel_send_async_data( channel_queue_entry_t* );
response_t dwhci_channel_send_async_ack( channel_queue_entry_t* );
response_t dwhci_channel_send_async_done( channel_queue_entry_t* );
response_t dwhci_channel_send_cancel( const channel_queue_entry_t* );
response_t dwhci_channel_send_cancel_done( channel_queue_entry_t* );
response_t dwhci_channel_async_continue( channel_queue_entry_t* );
response_t dwhci_channel_send_async( usb_control_message_t*, size_t, const usbd_control_message_t*, size_t );
response_t dwhci_channel_poll_async_data( channel_queue_entry_t* );
response_t dwhci_channel_poll_async_ack( channel_queue_entry_t* );
response_t dwhci_channel_poll_async_done( channel_queue_entry_t* );
response_t dwhci_channel_poll_async( usb_interrupt_poll_t*, size_t, const usbd_interrupt_message_t*, pid_t );
response_t dwhci_get_next_entry( channel_queue_entry_t** );
response_t dwhci_continue_next( channel_queue_entry_t* );
response_t dwhci_next_usb_pid( dwhci_channel_state_t, uint32_t, uint8_t* );
response_t dwhci_cancel_by_device( uint32_t );
response_t dwhci_power_on( void );
response_t dwhci_core_flush_tx_fifo( uint32_t );
response_t dwhci_core_flush_rx_fifo( void );
response_t dwhci_init( void );

#endif
