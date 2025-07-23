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

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <sys/bolthur.h>
#include "dwhci.h"
#include "dwhciroothub.h"

#include <sys/ioctl.h>

#include "../../libhcd.h"
#include "../../libiomem.h"
#include "../../libperipheral.h"
#include "../../storage/sd/util.h"

uint32_t dwhciroothub_root_hub_device_number = 0;

// disable a bunch of warnings necessary to build packed structures
// for usb communication
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpedantic"

static libusb_device_descriptor_t descriptor = {
  .descriptor_length = 0x12,
  .descriptor_type = LIBUSB_DESCRIPTOR_DEVICE,
  .usb_version = 0x0200,
  .class = LIBUSB_DEVICE_CLASS_HUB,
  .subclass = 0,
  .protocol = 0,
  .max_packet_size0 = 0,
  .vendor_id = 0,
  .product_id = 0,
  .version = 0x0100,
  .manufacturer = 0,
  .product = 1,
  .serial_number = 0,
  .configuration_count = 1,
};

struct {
  libusb_configuration_descriptor_t configuration;
  libusb_interface_descriptor_t interface;
  libusb_endpoint_descriptor_t endpoint;
} __packed configuration_descriptor = {
  .configuration = {
    .descriptor_length = 9,
    .descriptor_type = LIBUSB_DESCRIPTOR_CONFIGURATION,
    .total_length = 0x19,
    .interface_count = 1,
    .configuration_value = 1,
    .string_index = 0,
    .attributes = {
      .remote_wakeup = false,
      .self_powered = true,
      .reserved1 = 1,
    },
    .maximum_power = 0,
  },
  .interface = {
    .descriptor_length = 9,
    .descriptor_type = LIBUSB_DESCRIPTOR_INTERFACE,
    .number = 0,
    .alternate_setting = 0,
    .endpoint_count = 1,
    .class = LIBUSB_INTERFACE_CLASS_HUB,
    .subclass = 0,
    .protocol = 0,
    .string_index = 0,
  },
  .endpoint = {
    .descriptor_length = 7,
    .descriptor_type = LIBUSB_DESCRIPTOR_ENDPOINT,
    .endpoint_address = {
      .number = 1,
      .direction = LIBUSB_DIRECTION_IN,
    },
    .attributes = {
      .transfer = LIBUSB_TRANSFER_INTERRUPT,
    },
    .packet = {
      .max_size = 8,
    },
    .interval = 0xff,
  },
};

static libusb_string_descriptor_t string0 = {
  .descriptor_length = 4,
  .descriptor_type = LIBUSB_DESCRIPTOR_STRING,
  .data = {
    0x0409
  },
};

static libusb_string_descriptor_t string1 = {
  .descriptor_length = sizeof( u"USB 2.0 Root Hub" ) + 2,
  .descriptor_type = LIBUSB_DESCRIPTOR_STRING,
  .data = u"USB 2.0 Root Hub",
};

static libusb_hub_descriptor_t hub_descriptor = {
  .descriptor_length = 0x9,
  .descriptor_type = LIBUSB_DESCRIPTOR_HUB,
  .port_count = 1,
  .attributes = {
    .power_switching_mode = LIBUSB_HUB_PORT_CONTROL_GLOBAL,
    .compound = false,
    .over_current_protection = LIBUSB_HUB_PORT_CONTROL_GLOBAL,
    .think_time = 0,
    .indicators = false,
  },
  .power_good_delay = 0,
  .maximum_hub_power = 0,
  .data = { 0x01, 0xff, },
};

// enable warnings again
#pragma GCC diagnostic pop

/**
 * @fn int dwhciroothub_process(libusb_device_t*, libusb_pipe_address_t, void*, size_t, libusb_device_request_t*)
 * @brief Process root hub request
 * @param dev
 * @param pipe
 * @param buffer
 * @param buffer_length
 * @param request
 * @return
 */
int dwhciroothub_process(
  libusb_device_t* dev,
  const libusb_pipe_address_t pipe,
  void* buffer,
  const size_t buffer_length,
  libusb_device_request_t* request
) {
  // set device to processing
  dev->error = LIBUSB_TRANSFER_ERROR_PROCESSING;
  // check for interrupt pipe on root hub => not supported
  if ( LIBUSB_TRANSFER_INTERRUPT == pipe.type ) {
    // debug output
    #if defined ( DWHCI_ENABLE_DEBUG )
      STARTUP_PRINT( "Root hub does not support irq pipes\r\n" )
    #endif
    // set error
    dev->error = LIBUSB_TRANSFER_ERROR_STALL;
    // return success
    return 0;
  }
  // variable for result
  int result = 0;
  uint32_t reply_length = 0;
  response_t dwhci_result;
  uint32_t host_port;
  int ioctl_result;
  size_t sequence_size;
  iomem_mmio_entry_t* sequence;
  // handle request
  switch ( request->request ) {
    case LIBUSB_DEVICE_REQUEST_GET_STATUS:
      switch ( request->type ) {
        case 0x80: //
          {
            const uint16_t val = 1;
            memcpy(buffer, &val, sizeof( val ) );
            reply_length = 2;
          }
          break;
        case 0x81: // interface
        case 0x82: // endpoint
          {
            const uint16_t val = 1;
            memcpy(buffer, &val, sizeof( val ) );
            reply_length = 2;
          }
          break;
        case 0xa0: // class
          {
            const uint32_t val = 1;
            memcpy(buffer, &val, sizeof( val ) );
            reply_length = 4;
          }
          break;
        case 0xa3:
          // read host port
          dwhci_result = dwhci_read_host_port( &host_port );
          if ( HCD_RESPONSE_OK != dwhci_result ) {
            dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
            break;
          }
          STARTUP_PRINT( "host_port = %"PRIx32"\r\n", host_port )
          // push to local variable
          libusb_hub_port_full_status_t status = {
            .status = {
              // populate response
              .connected = host_port & HCD_DWHCI_HOST_PORT_CONNECT ? 1 : 0,
              .enabled = host_port & HCD_DWHCI_HOST_PORT_ENABLE ? 1 : 0,
              .suspended = host_port & HCD_DWHCI_HOST_PORT_SUSPEND ? 1 : 0,
              .over_current = host_port & HCD_DWHCI_HOST_PORT_OVERCURRENT ? 1 : 0,
              .reset = host_port & HCD_DWHCI_HOST_PORT_RESET ? 1 : 0,
              .power = host_port & HCD_DWHCI_HOST_PORT_POWER ? 1 : 0,
              .test_mode = HCD_DWHCI_HOST_PORT_TEST_CONTROL( host_port ) ? 1 : 0,
            },
            .change = {
              .connected_changed = host_port & HCD_DWHCI_HOST_PORT_CONNECT_CHANGED ? 1 : 0,
              .enabled_changed = host_port & HCD_DWHCI_HOST_PORT_ENABLE_CHANGED ? 1 : 0,
              .over_current_changed = host_port & HCD_DWHCI_HOST_PORT_OVERCURRENT_CHANGED ? 1 : 0,
              .reset_changed = true,
            },
          };
          if ( LIBUSB_SPEED_HIGH == HCD_DWHCI_HOST_PORT_SPEED( host_port ) ) {
            status.status.high_speed_attached = true;
          } else if ( LIBUSB_SPEED_LOW == HCD_DWHCI_HOST_PORT_SPEED( host_port ) ) {
            status.status.low_speed_attached = true;
          }
          // copy over to buffer
          const uint32_t val = 0;
          memcpy( buffer, &val, sizeof( val ) );
          memcpy( buffer, &status, sizeof( status ) );
          reply_length = 4;
          break;
        default:
          dev->error = LIBUSB_TRANSFER_ERROR_STALL;
      }
      break;
    case LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE:
      reply_length = 0;
      switch ( request->type ) {
        case 0x2:
        case 0x20:
          break;
        case 0x23:
          switch ( ( libusb_hub_port_feature_t)request->value ) {
            case LIBUSB_HUB_PORT_FEATURE_ENABLE:
              STARTUP_PRINT( "roothub port feature enable!\r\n" )
              // read host port
              dwhci_result = dwhci_read_host_port( &host_port );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              // set enable
              host_port |= HCD_DWHCI_HOST_PORT_ENABLE;
              // write back host port
              dwhci_result = dwhci_write_host_port( host_port | 0x4 );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            case LIBUSB_HUB_PORT_FEATURE_SUSPEND:
              STARTUP_PRINT( "roothub port feature suspend!\r\n" )
              // allocate sequence
              sequence = util_prepare_mmio_sequence( 7, &sequence_size );
              if ( ! sequence ) {
                dev->error = LIBUSB_TRANSFER_ERROR_BUFFER_ERROR;
                break;
              }
              // prepare sequence
              // clear power
              sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
              sequence[ 0 ].offset = PERIPHERAL_USB_POWER_OFFSET;
              sequence[ 0 ].value = 0;
              // delay 10 milliseconds
              sequence[ 1 ].type = IOMEM_MMIO_ACTION_SLEEP;
              sequence[ 1 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
              sequence[ 1 ].sleep = 10; // 5 according to specs
              // read host port
              sequence[ 2 ].type = IOMEM_MMIO_ACTION_READ_OR;
              sequence[ 2 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 2 ].value = HCD_DWHCI_HOST_PORT_RESUME;
              // write host port with resume enabled
              sequence[ 3 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
              sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 3 ].value = 0x40;
              // delay 200 milliseconds
              sequence[ 4 ].type = IOMEM_MMIO_ACTION_SLEEP;
              sequence[ 4 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
              sequence[ 4 ].sleep = 200; // 100 according to specs
              // read again host port
              sequence[ 5 ].type = IOMEM_MMIO_ACTION_READ_AND;
              sequence[ 5 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 5 ].value = ( uint32_t )~( HCD_DWHCI_HOST_PORT_RESUME
                | HCD_DWHCI_HOST_PORT_SUSPEND );
              // write back with suspend false
              sequence[ 6 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
              sequence[ 6 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 6 ].value = 0xc0;
              // execute sequence
              ioctl_result = ioctl(
                fd_iomem,
                IOCTL_BUILD_REQUEST(
                  IOMEM_RPC_MMIO_PERFORM,
                  sequence_size,
                  IOCTL_RDWR
                ),
                sequence
              );
              // free sequence
              free( sequence );
              // handle ioctl error
              if ( -1 == ioctl_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            case LIBUSB_HUB_PORT_FEATURE_POWER:
              STARTUP_PRINT( "roothub port feature power!\r\n" )
              // read host port
              dwhci_result = dwhci_read_host_port( &host_port );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              // reset power bit
              host_port &= ( uint32_t )~HCD_DWHCI_HOST_PORT_POWER;
              // write back host port
              dwhci_result = dwhci_write_host_port( host_port | 0x1000 );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            case LIBUSB_HUB_PORT_FEATURE_CONNECTION_CHANGE:
              STARTUP_PRINT( "roothub port feature connection change!\r\n" )
              // read host port
              dwhci_result = dwhci_read_host_port( &host_port );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              // set connect changed
              host_port |= HCD_DWHCI_HOST_PORT_CONNECT_CHANGED;
              // write back host port
              dwhci_result = dwhci_write_host_port( host_port | 0x2 );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            case LIBUSB_HUB_PORT_FEATURE_ENABLE_CHANGE:
              STARTUP_PRINT( "roothub port feature enable change!\r\n" )
              // read host port
              dwhci_result = dwhci_read_host_port( &host_port );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              // set enable changed
              host_port |= HCD_DWHCI_HOST_PORT_ENABLE_CHANGED;
              // write back host port
              dwhci_result = dwhci_write_host_port( host_port | 0x8 );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            case LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT_CHANGE:
              STARTUP_PRINT( "roothub port feature over current change!\r\n" )
              // read host port
              dwhci_result = dwhci_read_host_port( &host_port );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              // set over current changed
              host_port |= HCD_DWHCI_HOST_PORT_OVERCURRENT_CHANGED;
              // write back host port
              dwhci_result = dwhci_write_host_port( host_port | 0x20 );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            default:
          }
          break;
        default:
          result = EINVAL;
      }
      break;
    case LIBUSB_DEVICE_REQUEST_SET_FEATURE:
      switch ( request->type ) {
        case 0x20:
          break;
        case 0x23:
          switch ( ( libusb_hub_port_feature_t )request->value ) {
            case LIBUSB_HUB_PORT_FEATURE_RESET:
              STARTUP_PRINT( "roothub port feature reset!\r\n" )
              // allocate sequence
              sequence = util_prepare_mmio_sequence( 8, &sequence_size );
              if ( ! sequence ) {
                dev->error = LIBUSB_TRANSFER_ERROR_BUFFER_ERROR;
                break;
              }
              // read power with and
              sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ_AND;
              sequence[ 0 ].offset = PERIPHERAL_USB_POWER_OFFSET;
              sequence[ 0 ].value = ( uint32_t )~HCD_DWHCI_POWER_REGISTER_ENABLE_SLEEP_CLOCK_GATING;
              // write back power with disabled sleep clock and stop p clock
              sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
              sequence[ 1 ].offset = PERIPHERAL_USB_POWER_OFFSET;
              sequence[ 1 ].value = ( uint32_t )~HCD_DWHCI_POWER_REGISTER_STOP_P_CLOCK;
              // clear power
              sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
              sequence[ 2 ].offset = PERIPHERAL_USB_POWER_OFFSET;
              sequence[ 2 ].value = 0;
              // read port
              sequence[ 3 ].type = IOMEM_MMIO_ACTION_READ_AND;
              sequence[ 3 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 3 ].value = ( uint32_t )~HCD_DWHCI_HOST_PORT_SUSPEND;
              // write back power with enabled reset and power flag and disabled suspend flag
              sequence[ 4 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
              sequence[ 4 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 4 ].value = HCD_DWHCI_HOST_PORT_RESET | HCD_DWHCI_HOST_PORT_POWER | 0x1180;
              // delay 200 milliseconds
              sequence[ 5 ].type = IOMEM_MMIO_ACTION_SLEEP;
              sequence[ 5 ].sleep_type = IOMEM_MMIO_SLEEP_MILLISECONDS;
              sequence[ 5 ].sleep = 120; // 60 according to specs
              // read port
              sequence[ 6 ].type = IOMEM_MMIO_ACTION_READ_AND;
              sequence[ 6 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 6 ].value = ( uint32_t )~HCD_DWHCI_HOST_PORT_RESET;
              // write back previous read
              sequence[ 7 ].type = IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ;
              sequence[ 7 ].offset = PERIPHERAL_DWHCI_HOST_PORT;
              sequence[ 7 ].value = 0x1000;
              // execute sequence
              ioctl_result = ioctl(
                fd_iomem,
                IOCTL_BUILD_REQUEST(
                  IOMEM_RPC_MMIO_PERFORM,
                  sequence_size,
                  IOCTL_RDWR
                ),
                sequence
              );
              // free sequence
              free( sequence );
              // handle ioctl error
              if ( -1 == ioctl_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            case LIBUSB_HUB_PORT_FEATURE_POWER:
              STARTUP_PRINT( "roothub port feature power!\r\n" )
              // read host port
              dwhci_result = dwhci_read_host_port( &host_port );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              // set over current changed
              host_port |= HCD_DWHCI_HOST_PORT_POWER;
              // write back host port
              dwhci_result = dwhci_write_host_port( host_port | 0x1000 );
              if ( HCD_RESPONSE_OK != dwhci_result ) {
                dev->error = LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR;
                break;
              }
              break;
            default:
              break;
          }
          break;
        default:
          result = EINVAL;
      }
      break;
    case LIBUSB_DEVICE_REQUEST_SET_ADDRESS:
      reply_length = 0;
      dwhciroothub_root_hub_device_number = request->value;
      break;
    case LIBUSB_DEVICE_REQUEST_GET_DESCRIPTOR:
      switch ( request->type ) {
        case 0x80:
          switch ( ( libusb_descriptor_type_t )( ( request->value >> 8 ) & 0xFF ) ) {
            case LIBUSB_DESCRIPTOR_DEVICE:
              reply_length = ( uint32_t )fmin( sizeof( descriptor ), buffer_length );
              memcpy( buffer, &descriptor, reply_length );
              break;
            case LIBUSB_DESCRIPTOR_CONFIGURATION:
              reply_length = ( uint32_t )fmin( sizeof( configuration_descriptor ), buffer_length );
              memcpy( buffer, &configuration_descriptor, reply_length );
              break;
            case LIBUSB_DESCRIPTOR_STRING:
              switch ( request->value & 0xFF ) {
                case 0:
                  reply_length = ( uint32_t )fmin( string0.descriptor_length, buffer_length );
                  memcpy( buffer, &string0, reply_length );
                  break;
                case 1:
                  reply_length = ( uint32_t )fmin( string1.descriptor_length, buffer_length );
                  memcpy( buffer, &string1, reply_length );
                  break;
                default:
                  reply_length = 0;
              }
              break;
            default:
                result = EINVAL;
          }
          break;
        case 0xa0:
          reply_length = ( uint32_t )fmin( hub_descriptor.descriptor_length, buffer_length );
          memcpy( buffer, &hub_descriptor, reply_length );
          break;
        default:
            result = EINVAL;
      }
      break;
    case LIBUSB_DEVICE_REQUEST_GET_CONFIGURATION:
      *( ( uint8_t* )buffer ) = 0x1;
      reply_length = 1;
      break;
    case LIBUSB_DEVICE_REQUEST_SET_CONFIGURATION:
      reply_length = 0;
      break;
    default: result = EINVAL;
  }
  // handle invalid argument
  if ( EINVAL == result ) {
    dev->error |= LIBUSB_TRANSFER_ERROR_STALL;
  }
  // strip out processing error
  dev->error &= ( uint32_t )~LIBUSB_TRANSFER_ERROR_PROCESSING;
  // set last transfer
  dev->last_transfer = reply_length;
  // return success
  return 0;
}
