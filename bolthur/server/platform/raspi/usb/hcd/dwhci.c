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
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include <sys/_default_fcntl.h>
// local includes
#include "dwhci.h"
#include "util.h"
#include "response.h"
// driver includes
#include <iso646.h>
#include <sys/mman.h>

#include "../../libhcd.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../libmailbox.h"

/**
 * @brief file descriptor for iomem operations
 */
int fd_iomem = -1;

/**
 * @brief Data buffer used for data transfer
 */
void* databuffer = nullptr;

/**
 * @fn response_t dwhci_read_port(uint32_t, uint32_t*)
 * @brief Helper to read a port
 * @param port
 * @param value
 * @return
 */
response_t dwhci_read_port( const uint32_t port, uint32_t* value ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Query port information\r\n" )
  #endif
  // validate parameter
  if ( ! value ) {
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
  sequence[ 0 ].offset = port;
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
  *value = sequence[ 0 ].value;
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_write_port(uint32_t, uint32_t)
 * @brief Helper to write to a port
 * @param port
 * @param value
 * @return
 */
response_t dwhci_write_port( const uint32_t port, const uint32_t value ) {
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Query port information\r\n" )
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
  // overwrite register to read
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = port;
  sequence[ 0 ].value = value;
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
  // free sequence
  free( sequence );
  // return success
  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_transmit_channel(uint8_t, void*)
 * @brief Transmit channel operation
 * @param channel
 * @param buffer
 * @return
 */
response_t dwhci_transmit_channel( const uint8_t channel, void* buffer ) {
  // translate buffer to physical bus address
  const uintptr_t phys = _syscall_memory_translate_bus( ( uintptr_t )buffer, 1 );
  if ( errno ) {
    return HCD_RESPONSE_ERROR_IO;
  }
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 5, &sequence_size );
  if ( ! sequence ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }

  // read split control with unset of complete split
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 0 ].offset = ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel );
  sequence[ 0 ].value = ( uint32_t )~( HCD_DWHCI_CHAN_SPLIT_CONTROL_COMPLETE_SPLIT( true ) );
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
  const int result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Transmit channel sequence failed\r\n" )
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
 * @fn response_t dwhci_channel_interrupt_to_error(libusb_transfer_error_t*, uint8_t, bool)
 * @brief Translate channel interrupt to error
 * @param error
 * @param channel
 * @param completed
 * @return
 */
response_t dwhci_channel_interrupt_to_error( libusb_transfer_error_t* error, const uint8_t channel, const bool completed ) {
  uint32_t interrupt;
  response_t result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ), &interrupt );
  if ( HCD_RESPONSE_OK != result ) {
    return result;
  }
  result = HCD_RESPONSE_OK;
  if ( interrupt & HCD_CHANNEL_INTERRUPT_AHB_ERROR ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "AHB ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_AHB_ERROR;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_STALL ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "STALL ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_STALL;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "NEGATIVE ACK ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "ACK ERROR\r\n" )
    #endif
    result = HCD_RESPONSE_ERROR_TIMEOUT;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_NOT_YET ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "NOT_YET ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_NOT_YET_ERROR;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_BABBLE_ERROR ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "BABBLE_ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_BABBLE;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_FRAME_OVERRUN ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "OVERRUN ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_BUFFER_ERROR;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_DATA_TOGGLE_ERROR ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "DATA_TOGGLE_ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_BIT_ERROR;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( interrupt & HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "TRANSACTION_ERROR\r\n" )
    #endif
    *error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
    return HCD_RESPONSE_ERROR_UNKNOWN;
  }
  if ( !( interrupt & HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE ) && completed ) {
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "COMPLETED BUT FLAG NOT COMPLETED\r\n" )
    #endif
    result = HCD_RESPONSE_ERROR_TIMEOUT;
  }
  return result;
}

/**
 * @fn void custom_nanosleep(const struct timespec*)
 * @brief Custom nano sleep
 * @param rqtp
 */
static void custom_nanosleep( const struct timespec* rqtp ) {
  if ( 0 > rqtp->tv_nsec ) {
    errno = EINVAL;
    return;
  }
  // get clock frequency
  size_t frequency = _syscall_timer_frequency();
  // calculate second timeout
  size_t timeout = ( size_t )( rqtp->tv_sec * frequency );
  size_t tick;
  // add nanosecond offset
  timeout += ( size_t )( ( double )rqtp->tv_nsec * ( double )frequency / 1000000000.0 );
  // add tick count to get an end time
  timeout += _syscall_timer_tick_count();
  // loop until timeout is reached
  while ( ( tick = _syscall_timer_tick_count() ) < timeout ) {
    //#if defined( RPC_ENABLE_DEBUG )
    //  EARLY_STARTUP_PRINT( "sleeping %d / %d\r\n", tick, timeout )
    //#endif
    __asm__ __volatile__( "nop" );
  }
  //#if defined( RPC_ENABLE_DEBUG )
  //  EARLY_STARTUP_PRINT( "sleeping %d / %d\r\n", tick, timeout )
  //#endif
}

/**
 * @fn response_t dwhci_channel_send_wait_one(libusb_transfer_error_t*, uint8_t, void*, uint32_t, libusb_speed_t)
 * @brief Send on channel one and wait for response
 * @param error
 * @param channel
 * @param buffer
 * @param buffer_offset
 * @param speed
 * @return
 */
response_t dwhci_channel_send_wait_one(
  libusb_transfer_error_t* error,
  const uint8_t channel,
  void* buffer,
  const uint32_t buffer_offset,
  [[maybe_unused]] libusb_speed_t speed
) {
  uint32_t tries, global_tries, actual_tries;

  for (
    global_tries = 0, actual_tries = 0;
    global_tries < 3 && actual_tries < 10;
    global_tries++, actual_tries++
  ) {
    // clear interrupts
    response_t result = dwhci_write_port(
      ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ),
      0xffffffff
    );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }
    result = dwhci_write_port(
      ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( channel ),
      0
    );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }
    // fetch transfer size
    uint32_t transfer_size;
    result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ), &transfer_size );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }
    // fetch split control
    uint32_t split_control;
    result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ), &split_control );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }

    // transmit channel
    result = dwhci_transmit_channel( channel, ( uint8_t* )buffer + buffer_offset );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }

    uint32_t timeout = 0;
    do {
      // handle timeout
      if (timeout++ == 5000) {
        *error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
        return HCD_RESPONSE_ERROR_TIMEOUT;
      }

      uint32_t interrupt;
      result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ), &interrupt );
      if ( HCD_RESPONSE_OK != result ) {
        return result;
      }

      if ( interrupt & HCD_CHANNEL_INTERRUPT_HALT ) {
        #if defined ( DWHCI_ENABLE_DEBUG )
          STARTUP_PRINT( "Halt interrupt: %#"PRIx32"!\r\n", interrupt )
        #endif
        break;
      }

      const long milliseconds = 1;
      custom_nanosleep( &(struct timespec){
        .tv_sec = milliseconds / 1000,
        .tv_nsec = ( milliseconds % 1000 ) * 1000000,
      } );
    } while( true );

    result = dwhci_read_port(
      ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ),
      &transfer_size
    );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }

    uint32_t interrupt;
    result = dwhci_read_port(
      ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ),
        &interrupt );
    if ( HCD_RESPONSE_OK != result ) {
      return result;
    }

    if ( split_control & ( uint32_t)HCD_DWHCI_CHAN_SPLIT_CONTROL_EXTRACT_SPLIT_ENABLE ) {
      if ( interrupt & HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT ) {
        for ( tries = 0; tries < 3; tries++ ) {
          // clear interrupts
          result = dwhci_write_port(
            ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ),
            0xffffffff
          );
          if ( HCD_RESPONSE_OK != result ) {
            return result;
          }
          result = dwhci_write_port(
            ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT_MASK( channel ),
            0
          );
          if ( HCD_RESPONSE_OK != result ) {
            return result;
          }

          result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ), &split_control );
          if ( HCD_RESPONSE_OK != result ) {
            return result;
          }
          // unset complete split
          split_control &= ( uint32_t )~( HCD_DWHCI_CHAN_SPLIT_CONTROL_COMPLETE_SPLIT( true ) );
          // write back
          result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_SPLIT_CTRL( channel ), split_control );
          if ( HCD_RESPONSE_OK != result ) {
            return result;
          }
          // read characteristic
          uint32_t characteristic;
          result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ), &characteristic );
          if ( HCD_RESPONSE_OK != result ) {
            return result;
          }
          // unset packets per frame, enable and disable
          characteristic &= ( uint32_t )~(
            ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 )
            | ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_DISABLE( 1 )
          );
          // set packets per frame and enable
          characteristic |= ( uint32_t )HCD_DWHCI_CHAN_CHARACTER_ENABLE( 1 );
          // write back
          result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_CHARACTER( channel ), characteristic );
          if ( HCD_RESPONSE_OK != result ) {
            return result;
          }

          timeout = 0;
          do {
            // handle timeout
            if (timeout++ == 5000) {
              *error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
              return HCD_RESPONSE_ERROR_TIMEOUT;
            }

            result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_INT( channel ), &interrupt );
            if ( HCD_RESPONSE_OK != result ) {
              return result;
            }

            if ( interrupt & HCD_CHANNEL_INTERRUPT_HALT ) {
              break;
            }

            const long milliseconds = 1;
            custom_nanosleep( &(struct timespec){
              .tv_sec = milliseconds / 1000,
              .tv_nsec = ( milliseconds % 1000 ) * 1000000,
            } );
          } while( true );

          if ( ! ( interrupt & HCD_CHANNEL_INTERRUPT_NOT_YET ) ) {
            break;
          }
        }

        if ( tries == 3 ) {
          const long milliseconds = 25;
          custom_nanosleep( &(struct timespec){
            .tv_sec = milliseconds / 1000,
            .tv_nsec = ( milliseconds % 1000 ) * 1000000,
          } );
          continue;
        }
        if ( interrupt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT ) {
          global_tries--;
          const long milliseconds = 25;
          custom_nanosleep( &(struct timespec){
            .tv_sec = milliseconds / 1000,
            .tv_nsec = ( milliseconds % 1000 ) * 1000000,
          } );
          continue;
        }
        if ( interrupt & HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ) {
          const long milliseconds = 25;
          custom_nanosleep( &(struct timespec){
            .tv_sec = milliseconds / 1000,
            .tv_nsec = ( milliseconds % 1000 ) * 1000000,
          } );
          continue;
        }

        result = dwhci_channel_interrupt_to_error( error, channel, false );
        if ( HCD_RESPONSE_OK != result ) {
          return result;
        }
      } else if ( interrupt & HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT ) {
        global_tries--;
        const long milliseconds = 25;
        custom_nanosleep( &(struct timespec){
          .tv_sec = milliseconds / 1000,
          .tv_nsec = ( milliseconds % 1000 ) * 1000000,
        } );
        continue;
      } else if ( interrupt & HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ) {
        const long milliseconds = 25;
        custom_nanosleep( &(struct timespec){
          .tv_sec = milliseconds / 1000,
          .tv_nsec = ( milliseconds % 1000 ) * 1000000,
        } );
        continue;
      }
    } else {
      result = dwhci_channel_interrupt_to_error(
        error, channel, !( split_control & ( uint32_t )HCD_DWHCI_CHAN_SPLIT_CONTROL_EXTRACT_SPLIT_ENABLE ) );
      if ( HCD_RESPONSE_OK != result ) {
        return HCD_RESPONSE_RETRY;
      }
    }

    break;
  }

  if ( global_tries == 3 || actual_tries == 10 ) {
    *error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }

  return HCD_RESPONSE_OK;
}

/**
 * @fn response_t dwhci_prepare_channel(uint32_t, uint32_t, uint8_t, uint32_t, dwhci_channel_state_t, libusb_pipe_address_t*)
 * @brief Prepare channel for transfer
 * @param parent_device_number
 * @param port_number
 * @param channel
 * @param buffer_length
 * @param packet_id
 * @param pipe
 * @return
 */
response_t dwhci_prepare_channel(
  const uint32_t parent_device_number,
  const uint32_t port_number,
  const uint8_t channel,
  const uint32_t buffer_length,
  const dwhci_channel_state_t packet_id,
  libusb_pipe_address_t* pipe
) {
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "%d / %d / %"PRIu8" / %"PRIu8" / %d / %d\r\n",
      pipe->max_size, pipe->speed, pipe->end_point, pipe->device, pipe->type, pipe->direction
  #endif
  // prepare characteristic
  const uint32_t characteristic = HCD_DWHCI_CHAN_CHARACTER_DEVICE_ADDRESS( pipe->device )
    | HCD_DWHCI_CHAN_CHARACTER_END_POINT_NUMBER( pipe->end_point )
    | HCD_DWHCI_CHAN_CHARACTER_END_POINT_DIRECTION( pipe->direction )
    | HCD_DWHCI_CHAN_CHARACTER_LOW_SPEED( pipe->speed == LIBUSB_SPEED_LOW ? true : false )
    | HCD_DWHCI_CHAN_CHARACTER_TYPE( pipe->type )
    | HCD_DWHCI_CHAN_CHARACTER_MAXIMUM_PACKET_SIZE( usb_number_from_packet_size( pipe->max_size ) )
    | HCD_DWHCI_CHAN_CHARACTER_ENABLE( false )
    | HCD_DWHCI_CHAN_CHARACTER_DISABLE( false );
  // prepare split control
  uint32_t split_control = 0;
  if ( LIBUSB_SPEED_HIGH != pipe->speed ) {
    split_control = ( uint32_t )HCD_DWHCI_CHAN_SPLIT_CONTROL_SPLIT_ENABLE( true )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_HUB_ADDRESS( parent_device_number )
      | HCD_DWHCI_CHAN_SPLIT_CONTROL_PORT_ADDRESS( port_number );
  }
  // prepare transfer data
  uint32_t transfer_data =
    HCD_DWHCI_CHAN_XFER_SIZE_TRANSFER_SIZE( buffer_length )
    | HCD_DWHCI_CHAN_XFER_SIZE_PACKET_ID( packet_id );

  uint32_t packet_count = ( buffer_length + 7 ) / 8;
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "characteristic = %#"PRIx32", split_control = %#"PRIx32", transfer_data = %#"PRIx32", packet_count = %#"PRIx32"\r\n",
    characteristic, split_control, transfer_data, packet_count )
  #endif
  if ( LIBUSB_SPEED_LOW != pipe->speed ) {
    packet_count = (
      buffer_length + usb_number_from_packet_size( pipe->max_size ) - 1 ) / usb_number_from_packet_size( pipe->max_size );
  }
  if ( 0 == packet_count ) {
    packet_count = 1;
  }
  transfer_data |= HCD_DWHCI_CHAN_XFER_SIZE_PACKET_COUNT( packet_count );
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "characteristic = %#"PRIx32", split_control = %#"PRIx32", transfer_data = %#"PRIx32", packet_count = %#"PRIx32"\r\n",
      characteristic, split_control, transfer_data, packet_count )
  #endif
  // allocate mmio sequence
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate sequence size\r\n" )
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
  const int result = ioctl(
    fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MMIO_PERFORM,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle error
  if ( -1 == result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Transmit channel sequence failed\r\n" )
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
 * @fn response_t dwhci_channel_send_wait(uint32_t, uint32_t, libusb_transfer_error_t*, libusb_pipe_address_t*, uint8_t, void*, size_t, dwhci_channel_state_t, uint32_t*)
 * @brief Send command to channel and wait
 * @param parent_device_number
 * @param port_number
 * @param error
 * @param pipe
 * @param channel
 * @param buffer
 * @param buffer_length
 * @param packet_id
 * @param transfer_out
 * @return
 */
response_t dwhci_channel_send_wait(
  const uint32_t parent_device_number,
  const uint32_t port_number,
  libusb_transfer_error_t* error,
  libusb_pipe_address_t* pipe,
  const uint8_t channel,
  void* buffer,
  const size_t buffer_length,
  const dwhci_channel_state_t packet_id,
  uint32_t* transfer_out
) {
  uint32_t tries = 0;
  uint32_t packets;
  do {
    // prepare channel
    response_t result = dwhci_prepare_channel( parent_device_number,
      port_number, channel, buffer_length, packet_id, pipe );
    // handle error
    if ( HCD_RESPONSE_OK != result ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to prepare the channel\r\n" )
      #endif
      // return result
      return result;
    }
    size_t transferred = 0;
    uint32_t transfer_data;
    bool retry_error = false;
    do {
      // read port
      result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ), &transfer_data );
      // handle error
      if ( HCD_RESPONSE_OK != result ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          STARTUP_PRINT( "Unable to read port data\r\n" )
        #endif
        // return result
        return result;
      }
      // set packets
      packets = HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_PACKET_COUNT( transfer_data );
      // transfer
      result = dwhci_channel_send_wait_one( error, channel, buffer, transferred, pipe->speed );
      // handle retry
      if ( HCD_RESPONSE_RETRY == result ) {
        retry_error = true;
        break;
      }
      if ( HCD_RESPONSE_OK != result ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          STARTUP_PRINT( "Unable to transfer data\r\n" )
        #endif
        // return result
        return result;
      }
      // read port
      result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ), &transfer_data );
      // handle error
      if ( HCD_RESPONSE_OK != result ) {
        // debug output
        #if defined( DWHCI_ENABLE_DEBUG )
          STARTUP_PRINT( "Unable to read port data\r\n" )
        #endif
        // return result
        return result;
      }
      #if defined ( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "transferred = %#zx, packets = %"PRIu32"\r\n", transferred, packets );
      #endif
      transferred = buffer_length - HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_TRANSFER_SIZE( transfer_data );
      #if defined ( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "transferred = %#zx, packets = %"PRIu32"\r\n", transferred, HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_PACKET_COUNT( transfer_data ) );
      #endif
      if ( packets == HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_PACKET_COUNT( transfer_data ) ) {
        break;
      }
    } while ( HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_PACKET_COUNT( transfer_data ) > 0 );

    // handle retry error
    if ( retry_error ) {
      continue;
    }

    // read port
    result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CHAN_XFER_SIZE( channel ), &transfer_data );
    // handle error
    if ( HCD_RESPONSE_OK != result ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Unable to read port data\r\n" )
      #endif
      // return result
      return result;
    }
    // handle stuck
    if (
      packets == HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_PACKET_COUNT( transfer_data )
      && buffer_length != 0
    ) {
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Reading device got stuck %"PRIu32" / %"PRIu32"\r\n",
          packets, HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_PACKET_COUNT( transfer_data ) )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_UNKNOWN;
    }
    *transfer_out = HCD_DWHCI_CHAN_XFER_SIZE_EXTRACT_TRANSFER_SIZE( transfer_data );
    // return success
    return HCD_RESPONSE_OK;
  } while ( tries++ < 3 );
  // return timeout
  return HCD_RESPONSE_ERROR_TIMEOUT;
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
  const uint32_t reset = HCD_DWHCI_CORE_RESET_TX_FIFO_FLUSH
   | ( num_fifo << HCD_DWHCI_CORE_RESET_TX_FIFO_NUM_SHIFT );

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
 * @fn response_t dwhci_init(void)
 * @brief Init dwhci otg
 * @return
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
  // allocate data buffer
  databuffer = mmap( NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_BUS | MAP_DEVICE , -1, 0 );
  if ( MAP_FAILED == databuffer ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to allocate data buffer\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // query vendor and hardware information
  size_t sequence_size;
  iomem_mmio_entry_t* sequence = util_prepare_mmio_sequence( 7, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
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
  int result = ioctl(
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
  const uint32_t hw_cfg2 = sequence[ 3 ].value;
  [[maybe_unused]] const uint32_t hw_cfg3 = sequence[ 4 ].value;
  [[maybe_unused]] const uint32_t hw_cfg4 = sequence[ 5 ].value;
  uint32_t host_cfg = sequence[ 6 ].value;
  // free sequence
  free( sequence );
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
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT(
      "HCD: Hardware: %c%c%"PRIx32".%"PRIx32"%"PRIx32"%"PRIx32" (BCM%.5"PRIx32")\r\n",
      ( char )( vendor >> 24 & 0xff ),
      ( char )( vendor >> 16 & 0xff ),
      vendor >> 12 & 0xf,
      vendor >> 8 & 0xf,
      vendor >> 4 & 0xf,
      vendor >> 0 & 0xf,
      user >> 12 & 0xffff
    )
    STARTUP_PRINT( "Hardware configuration: %#08"PRIx32" %#08"PRIx32" %#08"PRIx32" %#08"PRIx32"\r\n",
      hw_cfg1, hw_cfg2, hw_cfg3, hw_cfg4 )
    STARTUP_PRINT( "Host configuration: %#08"PRIx32"\r\n", host_cfg )
  #endif
  // check architecture
  if ( HCD_DWHCI_CORE_HW_CFG2_ARCHITECTURE( hw_cfg2 ) != HCD_DWHCI_CORE_HW_CFG2_ARCHITECTURE_INTERNAL_DMA ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Host architecture is not internal DMA\r\n" )
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
      STARTUP_PRINT( "High speed physical not supported\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return incompatible
    return HCD_RESPONSE_ERROR_INCOMPATIBLE;
  }*/
  // disable interrupts
  sequence = util_prepare_mmio_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
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
  result = ioctl(
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
      STARTUP_PRINT( "Disable of interrupts failed\r\n" )
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
  result = dwhci_power_on();
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to power on hub\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Disable pulse and vbus and perform initial resetr\r\n" )
  #endif
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 7, &sequence_size );
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
  // perform request
  result = ioctl(
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
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 2 ].abort_type ) {
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
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 5 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Wait for reset timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return timeout
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence
  free( sequence );

  // debug output
  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Phy initialization with reset\r\n" )
  #endif
  // allocate sequence
  sequence = util_prepare_mmio_sequence( 7, &sequence_size );
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
  sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_CORE_USB_CFG_PHYIF;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_USB_CFG;
  sequence[ 1 ].value = HCD_DWHCI_CORE_USB_CFG_ULPI_UTMI_SEL;
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
  // perform request
  result = ioctl(
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
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 2 ].abort_type ) {
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
  if ( IOMEM_MMIO_ABORT_TYPE_TIMEOUT == sequence[ 5 ].abort_type ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Wait for reset timed out\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return timeout
    return HCD_RESPONSE_ERROR_TIMEOUT;
  }
  // free sequence
  free( sequence );

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Preparing usb configuration\r\n" )
  #endif
  // query standalone register
  uint32_t usb_cfg;
  result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_USB_CFG, &usb_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read port failed\r\n" )
    #endif
    return result;
  }
  if (
    HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
    && HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
  ) {
    // enable configuration
    usb_cfg |= HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS | HCD_DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M;
  }
  else {
    // disable configuration
    usb_cfg &= (uint32_t)~( HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS | HCD_DWHCI_CORE_USB_CFG_ULPI_CLK_SUS_M );
  }
  // write back value
  result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_USB_CFG, usb_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Write port failed\r\n" )
    #endif
    return result;
  }

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Preparing dma configuration\r\n" )
  #endif
  // prepare sequence
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
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_CORE_AHB_CFG_GLOBAL_AHB_SINGLE;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = HCD_DWHCI_CORE_AHB_CFG_GLOBAL_DMA_ENABLE;
  // perform request
  result = ioctl(
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
  free( sequence );

  // query usb port again
  result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_USB_CFG, &usb_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read port failed\r\n" )
    #endif
    return result;
  }
  switch ( HCD_DWHCI_CORE_HW_CFG2_OP_MODE( hw_cfg2 ) ) {
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_HNP_SRP_CAPABLE:
      usb_cfg |= HCD_DWHCI_CORE_USB_CFG_SRP_CAPABLE | HCD_DWHCI_CORE_USB_CFG_HNP_CAPABLE;
      break;
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_SRP_ONLY_CAPABLE:
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_SRP_CAPABLE_DEVICE:
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_SRP_CAPABLE_HOST:
      usb_cfg &= (uint32_t)~HCD_DWHCI_CORE_USB_CFG_HNP_CAPABLE;
      usb_cfg |= HCD_DWHCI_CORE_USB_CFG_SRP_CAPABLE;
      break;
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_NO_HNP_SRP_CAPABLE:
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_NO_SRP_CAPABLE_DEVICE:
    case HCD_DWHCI_CORE_HW_CFG2_OP_MODE_NO_SRP_CAPABLE_HOST:
    default:
      usb_cfg &= (uint32_t)~HCD_DWHCI_CORE_USB_CFG_HNP_CAPABLE;
      usb_cfg &= (uint32_t)~HCD_DWHCI_CORE_USB_CFG_SRP_CAPABLE;
      break;
  }
  result = dwhci_write_port( ( uint32_t )PERIPHERAL_DWHCI_CORE_USB_CFG, usb_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Write port failed\r\n" )
    #endif
    return result;
  }

  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Preparing host startup\r\n" )
  #endif
  // prepare sequence
  sequence = util_prepare_mmio_sequence( 10, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // power down
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_USB_POWER_OFFSET;
  sequence[ 0 ].value = 0;
  // set clock rate
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_MASK;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 2 ].value = HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_HS_PHY_TYPE_ULPI
    && HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE( hw_cfg2 ) == HCD_DWHCI_CORE_HW_CFG2_FS_PHY_TYPE_DEDICATED
    && usb_cfg & HCD_DWHCI_CORE_USB_CFG_ULPI_FSLS
      ? HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_48_MHZ
      : HCD_DWHCI_HOST_CFG_FSLS_PCLK_SEL_30_60_MHZ;
  // enable fsls only
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 4 ].offset = PERIPHERAL_DWHCI_HOST_CFG;
  sequence[ 4 ].value = HCD_DWHCI_HOST_CFG_FSLS_ONLY;
  // write fifo size
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 5 ].offset = PERIPHERAL_DWHCI_CORE_RX_FIFO_SIZ;
  sequence[ 5 ].value = PERIPHERAL_DWHCI_DATA_FIFO_SIZE;
  // write nper fifo size
  sequence[ 6 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 6 ].offset = PERIPHERAL_DWHCI_CORE_NPER_FIFO_SIZ;
  sequence[ 6 ].value = HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE | ( HCD_DWHCI_CFG_HOST_NPER_TX_FIFO_SIZE << 16 );;
  sequence[ 7 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 7 ].offset = PERIPHERAL_DWHCI_CORE_HOST_PER_TX_FIFO_SZ;
  sequence[ 7 ].value = ( HCD_DWHCI_CFG_HOST_RX_FIFO_SIZE + HCD_DWHCI_CFG_HOST_NPER_TX_FIFO_SIZE )
    | HCD_DWHCI_CFG_HOST_PER_TX_FIFO_SIZE << 16;
  // enable otg
  sequence[ 8 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 8 ].offset = PERIPHERAL_DWHCI_CORE_CTRL;
  sequence[ 9 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 9 ].offset = PERIPHERAL_DWHCI_CORE_CTRL;
  sequence[ 9 ].value = HCD_DWHCI_CORE_CTRL_HOST_SET_NP_ENABLE;
  // perform request
  result = ioctl(
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
      STARTUP_PRINT( "host config sequence failed\r\n" )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  free( sequence );

  result = dwhci_core_flush_tx_fifo( 16 );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Flushing tx fifo failed\r\n" )
    #endif
    return result;
  }

  result = dwhci_core_flush_rx_fifo();
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Flushing rx fifo failed\r\n" )
    #endif
    return result;
  }


  // read out host config
  result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_CFG, &host_cfg );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Read of host cfg failed: %s\r\n", response_error( result ) )
    #endif
    // return error
    return result;
  }
  // put channels into known states
  if ( host_cfg & HCD_DWHCI_HOST_CFG_ENABLE_DMA_DESCRIPTOR ) {
    // prepare sequence
    sequence = util_prepare_mmio_sequence( 2, &sequence_size );
    if ( ! sequence ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_MEMORY;
    }
    // extract channel count
    const uint32_t channel_count = HCD_DWHCI_CORE_HW_CFG2_NUM_HOST_CHANNELS( hw_cfg2 );
    // loop over channels
    for ( uint32_t channel = 0; channel < channel_count; ++channel ) {
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
      result = ioctl(
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
          STARTUP_PRINT( "host config sequence failed\r\n" )
        #endif
        // free sequence
        free( sequence );
        // return error
        return HCD_RESPONSE_ERROR_IO;
      }
    }
    // free sequence again
    free( sequence );
    // prepare sequence
    sequence = util_prepare_mmio_sequence( 3, &sequence_size );
    if ( ! sequence ) {
      // debug output
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
      #endif
      // return error
      return HCD_RESPONSE_ERROR_MEMORY;
    }
    // loop through channels again
    for ( uint32_t channel = 0; channel < channel_count; ++channel ) {
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
      result = ioctl(
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
          STARTUP_PRINT( "host config sequence failed\r\n" )
        #endif
        // free sequence
        free( sequence );
        // return error
        return HCD_RESPONSE_ERROR_IO;
      }
      // check for timeout
      if ( sequence[ 2 ].abort_type == IOMEM_MMIO_ABORT_TYPE_TIMEOUT ) {
        #if defined( DWHCI_ENABLE_DEBUG )
          STARTUP_PRINT( "Unable to clear halt on channel %"PRIu32"\r\n",
            channel )
        #endif
      }
    }
    // free sequence
    free( sequence );
  }

  uint32_t host_port;
  result = dwhci_read_port( ( uint32_t )PERIPHERAL_DWHCI_HOST_PORT, &host_port );
  if ( HCD_RESPONSE_OK != result ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "host port read failed\r\n" )
    #endif
    return result;
  }
  if ( ! ( host_port & HCD_DWHCI_HOST_PORT_POWER ) ) {
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Power up host port\r\n" )
    #endif
    host_port |= HCD_DWHCI_HOST_PORT_POWER;
    result = dwhci_write_port( PERIPHERAL_DWHCI_HOST_PORT, host_port );
    if ( HCD_RESPONSE_OK != result ) {
      #if defined( DWHCI_ENABLE_DEBUG )
        STARTUP_PRINT( "host port write failed\r\n" )
      #endif
      return result;
    }
  }

  #if defined ( DWHCI_ENABLE_DEBUG )
    STARTUP_PRINT( "Resetting host port\r\n" )
  #endif
  // prepare sequence
  sequence = util_prepare_mmio_sequence( 5, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Sequence memory allocation failed\r\n" )
    #endif
    // return error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // set host port reset flag and write it back
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_OR;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 0 ].value = HCD_DWHCI_HOST_PORT_RESET;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 1 ].value = 0x100;
  // delay 100 ms
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_SLEEP;
  sequence[ 2 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
  sequence[ 2 ].sleep = 100;
  // reset reset flag
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ_AND;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 3 ].value = ( uint32_t )~HCD_DWHCI_HOST_PORT_RESET;
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 4 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
  sequence[ 4 ].value = 0x100;
  // perform request
  result = ioctl(
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
      STARTUP_PRINT( "host config sequence failed: %s\r\n", strerror( errno ) )
    #endif
    // free sequence
    free( sequence );
    // return error
    return HCD_RESPONSE_ERROR_IO;
  }
  // free sequence
  free( sequence );
  // register interrupt
  _syscall_interrupt_acquire( ARM_IRQ_USB );
  // handle error
  if ( errno ) {
    const int e = errno;
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to acquire interrupt %d: %s\r\n",
        ARM_IRQ_USB, strerror( e ) )
    #endif
    // return error
    return e;
  }
  // enable all interrupts
  sequence = util_prepare_mmio_sequence( 6, &sequence_size );
  if ( ! sequence ) {
    // debug output
    #if defined( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to prepare mmio_sequence\r\n" )
    #endif
    // close file descriptor
    close( fd_iomem );
    // return memory error
    return HCD_RESPONSE_ERROR_MEMORY;
  }
  // mask all pending interrupts
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_DWHCI_CORE_INT_STAT;
  sequence[ 0 ].value = ( uint32_t )-1;
  // enable core interrupts
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_READ_OR;
  sequence[ 1 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  sequence[ 1 ].value = HCD_DWHCI_CORE_AHB_CFG_GLOBAL_INTERRUPT_MASK;
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ;
  sequence[ 2 ].offset = PERIPHERAL_DWHCI_CORE_AHB_CFG;
  // enable host interrupts
  sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 3 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 3 ].value = 0;
  sequence[ 4 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 4 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 5 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
  sequence[ 5 ].offset = PERIPHERAL_DWHCI_CORE_INT_MASK;
  sequence[ 5 ].value = HCD_DWHCI_CORE_INT_MASK_HC_INTR
    | HCD_DWHCI_CORE_INT_MASK_PORT_INTR
    | HCD_DWHCI_CORE_INT_MASK_DISCONNECT
    | HCD_DWHCI_CORE_INT_MASK_USB_SUSPEND;
  // perform request
  result = ioctl(
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
      STARTUP_PRINT( "Enable of interrupts failed\r\n" )
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
  // return success, we're done
  return HCD_RESPONSE_OK;
}
