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
#include <errno.h>
#include <inttypes.h>
#include <sys/bolthur.h>
// local includes
#include "../../rpc.h"
#include "../../global.h"
#include "../../keyboard.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/hid/hid.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn void rpc_keyboard_attach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler attach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_keyboard_attach(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  // handle no data
  if( ! data_info ) {
    STARTUP_PRINT( "NO DATA PASSED!\r\n" )
    _syscall_rpc_cleanup();
    return;
  }
  // validate origin
  if (
    origin != allowed_rpc_origin
    && ! bolthur_rpc_validate_origin( origin, data_info )
  ) {
    STARTUP_PRINT( "INVALID ORIGIN!\r\n" )
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( ! request ) {
    STARTUP_PRINT( "ERROR WHILE FETCHING DATA: %s!\r\n", strerror( errno ) )
    _syscall_rpc_cleanup();
    return;
  }
  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;
  // get hid driver
  uint32_t device_driver;
  int result = hid_get_driver( message->device_number, &device_driver );
  if ( 0 != result ) {
    STARTUP_PRINT( "Error while fetching driver: %s\r\n", strerror( result ) )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // handle invalid device driver
  if ( device_driver != DEVICE_DRIVER_HID ) {
    STARTUP_PRINT( "\"%s\" is not a hid device. Keyboard driver is build upon hid driver\r\n",
      usb_get_description( message->device_number ) )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // get application
  libusb_hid_full_usage_t application;
  result = hid_get_application( message->device_number, &application );
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get application data: %s\r\n", strerror( result ) )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // check application data
  if (
    (
      application.page != LIBUSB_HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROL
      && application.page != LIBUSB_HID_USAGE_PAGE_UNDEFINED
    ) || application.desktop != LIBUSB_HID_USAGE_PAGE_DESKTOP_KEYBOARD
  ) {
    STARTUP_PRINT( "\"%s\" does not seem to be a keyboard\r\n",
      usb_get_description( message->device_number ) )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // get report count
  uint8_t report_count;
  result = hid_get_report_count( message->device_number, &report_count );
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get report count of device: %s\r\n",
      strerror( result ) )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // check report count
  if ( 0 >= report_count ) {
    STARTUP_PRINT( "\"%s\" does not have enough outputs to be a keyboard\r\n",
      usb_get_description( message->device_number ) )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // allocate device
  libusb_keyboard_device_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    STARTUP_PRINT( "Unable to allocate memory for device\r\n" )
    free( request );
    _syscall_rpc_cleanup();
    return;
  }
  // clear out keyboard driver space
  memset( device, 0, sizeof( *device ) );
  // populate header
  device->header.device_driver = DEVICE_DRIVER_KEYBOARD;
  device->header.data_size = sizeof( *device );
  // determine new index
  result = keyboard_new_index( &device->index );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to determine new index: %s\r\n", strerror( result ) )
    free( request );
    keyboard_destroy( device );
    _syscall_rpc_cleanup();
    return;
  }
  // iterate over reports
  for ( uint8_t idx = 0; idx < report_count; ++idx ) {
    // query report from hid
    libusb_hid_parser_report_t* report;
    result = hid_get_report( message->device_number, idx, &report );
    // handle error
    if ( 0 != result ) {
      STARTUP_PRINT( "Unable to get report %"PRIu8" from hid\r\n", idx )
      free( request );
      keyboard_destroy( device );
      _syscall_rpc_cleanup();
      return;
    }
    // some debug output
    STARTUP_PRINT( "type = %x, report = %"PRIu8", fields = %"PRIu8"\r\n",
      report->type, idx, report->field_count )
    // handle input
    if ( report->type == LIBUSB_HID_REPORT_TYPE_INPUT && ! device->key_report ) {
      for ( uint8_t inner = 0; inner < report->field_count; ++inner ) {
        if (
          report->fields[ inner ].usage.page == LIBUSB_HID_USAGE_PAGE_KEYBOARD_CONTROL
          || report->fields[ inner ].usage.page == LIBUSB_HID_USAGE_PAGE_UNDEFINED
        ) {
          if ( report->fields[ inner ].attribute.variable ) {
            if (
              report->fields[ inner ].usage.keyboard >= LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_CONTROL
              && report->fields[ inner ].usage.keyboard <= LIBUSB_HID_USAGE_PAGE_KEYBOARD_RIGHT_CONTROL
            ) {
              STARTUP_PRINT(
                "Modifier %d detected. Offset = %"PRIx8", size = %"PRIx8"\r\n",
                report->fields[ inner ].usage.keyboard,
                report->fields[ inner ].offset,
                report->fields[ inner ].size
              )
              // allocate space
              const size_t key_field_index = report->fields[ inner ].usage.keyboard - LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_CONTROL;
              result = keyboard_duplicate_report( &device->key_field[ key_field_index ], &report->fields[ inner ] );
              // handle error
              if ( 0 != result ) {
                STARTUP_PRINT( "Unable to duplicate report\r\n" )
                free( request );
                keyboard_destroy( device );
                _syscall_rpc_cleanup();
                return;
              }
            }
          } else {
            STARTUP_PRINT( "Key input detected\r\n" )
            result = keyboard_duplicate_report( &device->key_field[ 8 ], &report->fields[ inner ] );
            // handle error
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
          }
        }
      }
    } else if ( report->type == LIBUSB_HID_REPORT_TYPE_OUTPUT && ! device->led_report ) {
      // data->LedReport = parse->Report[i];
      for ( uint8_t inner = 0; inner < report->field_count; ++inner ) {
        // skip non led stuff
        if ( report->fields[ inner ].usage.page != LIBUSB_HID_USAGE_PAGE_LED ) {
          continue;
        }
        // handle led page
        switch ( report->fields[ inner ].usage.led ) {
          case LIBUSB_HID_USAGE_PAGE_LED_NUMBER_LOCK:
            STARTUP_PRINT( "Number lock led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 0 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.num_lock = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_CAPSLOCK:
            STARTUP_PRINT( "Capslock lock led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 1 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.caps_lock = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_SCROLL_LOCK:
            STARTUP_PRINT( "Scroll lock led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 2 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.scroll_lock = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_COMPOSE:
            STARTUP_PRINT( "Compose led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 3 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.compose = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_KANA:
            STARTUP_PRINT( "Kana led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 4 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.kana = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_POWER:
            STARTUP_PRINT( "Power led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 5 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.power = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_SHIFT:
            STARTUP_PRINT( "Shift led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 6 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.shift = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_MUTE:
            STARTUP_PRINT( "Mute led detected\r\n")
            // duplicate report
            result = keyboard_duplicate_report( &device->led_field[ 7 ], &report->fields[ inner ] );
            if ( 0 != result ) {
              STARTUP_PRINT( "Unable to duplicate report\r\n" )
              free( request );
              keyboard_destroy( device );
              _syscall_rpc_cleanup();
              return;
            }
            // set supported flag
            device->led.mute = true;
            break;
          default:
            break;
        }
      }
    }
    /// FIXME: IMPLEMENT
    // free report again
    free( report );
  }
  // append device to list
  keyboard_append( device );
  // free request and cleanup
  free( request );
  _syscall_rpc_cleanup();
}
