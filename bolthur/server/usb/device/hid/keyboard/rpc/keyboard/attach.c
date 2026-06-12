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
 *
 * @todo destroy fetched hid report correctly
 */
void rpc_keyboard_attach(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  EARLY_STARTUP_PRINT( "keyboard attach\r\n" )
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL, };
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox(
    data_info, &data_size, true, NULL );
  if ( ! request ) {
    err_response.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // allocate space for pull_request
  const usb_generic_attach_t* message = ( usb_generic_attach_t* )request->container;
  // get hid driver
  uint32_t device_driver;
  int result = hid_get_driver( message->device_number, &device_driver );
  if ( 0 != result ) {
    err_response.status = -ENODEV;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // handle invalid device driver
  if ( device_driver != DEVICE_DRIVER_HID ) {
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // get application
  libusb_hid_full_usage_t application;
  result = hid_get_application( message->device_number, &application );
  if ( 0 != result ) {
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // check application data
  if (
    (
      application.page != LIBUSB_HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROL
      && application.page != LIBUSB_HID_USAGE_PAGE_UNDEFINED
    ) || application.desktop != LIBUSB_HID_USAGE_PAGE_DESKTOP_KEYBOARD
  ) {
    err_response.status = -EBADMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // get report count
  uint8_t report_count = 0;
  result = hid_get_report_count( message->device_number, &report_count );
  if ( 0 != result ) {
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  for (uint8_t i = 0; i < report_count; i++ ) {
    // get endpoint information
    libusb_endpoint_descriptor_t descriptor;
    result = usb_get_endpoint(
      message->device_number, message->interface_number, i, &descriptor );
    // handle error
    if ( 0 != result ) {
      err_response.status = -result;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
      free( request );
      return;
    }
    #if defined( KEYBOARD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "descriptor.endpoint_address.number = %"PRIu8", descriptor.endpoint_address.direction = %d\r\n",
        descriptor.endpoint_address.number, descriptor.endpoint_address.direction)
    #endif
  }
  #if defined( KEYBOARD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "report_count = %"PRIu8"\r\n", report_count )
  #endif
  // get endpoint information
  libusb_endpoint_descriptor_t endpoint_descriptor;
  result = usb_get_endpoint(
    message->device_number, message->interface_number, 0, &endpoint_descriptor );
  // handle error
  if ( 0 != result ) {
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    free( request );
    return;
  }
  // check report count
  if ( 0 == report_count ) {
    free( request );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // allocate device
  libusb_keyboard_device_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    free( request );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // clear out keyboard driver space
  memset( device, 0, sizeof( *device ) );
  // populate header
  device->header.device_driver = DEVICE_DRIVER_KEYBOARD;
  device->header.data_size = sizeof( *device );
  memcpy( &device->descriptor, &endpoint_descriptor, sizeof( endpoint_descriptor ) );
  device->last_poll = 0;
  device->running_poll = 0;
  device->device_number = message->device_number;
  // determine new index
  result = keyboard_new_index( &device->index );
  // handle error
  if ( 0 != result ) {
    free( request );
    keyboard_destroy( device );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  // iterate over reports
  for ( uint8_t idx = 0; idx < report_count; ++idx ) {
    // query report from hid
    libusb_hid_parser_report_t* report;
    result = hid_get_report( message->device_number, idx, &report );
    // handle error
    if ( 0 != result ) {
      free( request );
      keyboard_destroy( device );
      err_response.status = -result;
      bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
      return;
    }
    // some debug output
    #if defined( KEYBOARD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "type = %x, report = %"PRIu8", fields = %"PRIu8"\r\n",
        report->type, idx, report->field_count )
    #endif
    // handle input
    if ( report->type == LIBUSB_HID_REPORT_TYPE_INPUT && ! device->key_report ) {
      // change idle state to only on key change
      #if defined( KEYBOARD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Setting idle to 0 for %"PRIu32" / %"PRIu32" / %"PRIu8"\r\n",
          message->device_number, message->interface_number, report->id )
      #endif
      result = hid_set_idle( message->device_number, message->interface_number, report->id, 0);
      if ( 0 != result ) {
        #if defined( KEYBOARD_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "Unable to put hid into idle mode: %s\r\n",
            strerror( result ) )
        #endif
        free( request );
        hid_destroy_report( report );
        keyboard_destroy( device );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
        return;
      }
      #if defined( KEYBOARD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Setting idle to 0 for %"PRIu32" / %"PRIu32" / %"PRIu8" done\r\n",
          message->device_number, message->interface_number, report->id )
      #endif
      // duplicate report
      result = keyboard_duplicate_report(
        &device->key_report,
        report,
        sizeof( libusb_hid_parser_report_t ) + report->field_count * sizeof( libusb_hid_parser_fields_t )
      );
      // handle error
      if ( 0 != result ) {
        free( request );
        hid_destroy_report( report );
        keyboard_destroy( device );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
        return;
      }
      // loop through reports
      for ( uint8_t inner = 0; inner < report->field_count; ++inner ) {
        #if defined( KEYBOARD_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "inner = %"PRIu8" / %#x\r\n", inner, report->fields[ inner ].usage.page )
        #endif
        // handle report field usage page
        if (
          report->fields[ inner ].usage.page == LIBUSB_HID_USAGE_PAGE_KEYBOARD_CONTROL
          || report->fields[ inner ].usage.page == LIBUSB_HID_USAGE_PAGE_UNDEFINED
        ) {
          if ( report->fields[ inner ].attribute.variable ) {
            if (
              report->fields[ inner ].usage.keyboard >= LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_CONTROL
              && report->fields[ inner ].usage.keyboard <= LIBUSB_HID_USAGE_PAGE_KEYBOARD_RIGHT_CONTROL
            ) {
              #if defined( KEYBOARD_ENABLE_DEBUG )
                EARLY_STARTUP_PRINT(
                  "Modifier %d detected. Offset = %"PRIx8", size = %"PRIx8"\r\n",
                  report->fields[ inner ].usage.keyboard,
                  report->fields[ inner ].offset,
                  report->fields[ inner ].size
                )
              #endif
              // allocate space
              const size_t key_field_index = report->fields[ inner ].usage.keyboard - LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_CONTROL;
              device->key_field[ key_field_index ] = &device->key_report->fields[ inner ];
            }
          } else {
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Key input detected: %p / %p / %"PRIu8" / %#x\r\n", (void*)&device->key_report->fields[ inner ],
                device->key_report->fields[ inner ].value.ptr, inner, report->fields[ inner ].usage.page )
            #endif
            device->key_field[ 8 ] = &device->key_report->fields[ inner ];
          }
        }
      }
    } else if ( report->type == LIBUSB_HID_REPORT_TYPE_OUTPUT && ! device->led_report ) {
      // duplicate report
      result = keyboard_duplicate_report(
        &device->led_report,
        report,
        sizeof( libusb_hid_parser_report_t ) + report->field_count * sizeof( libusb_hid_parser_fields_t )
      );
      // handle error
      if ( 0 != result ) {
        free( request );
        hid_destroy_report( report );
        keyboard_destroy( device );
        err_response.status = -result;
        bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
        return;
      }
      for ( uint8_t inner = 0; inner < report->field_count; ++inner ) {
        // skip non led stuff
        if ( report->fields[ inner ].usage.page != LIBUSB_HID_USAGE_PAGE_LED ) {
          continue;
        }
        // handle led page
        switch ( report->fields[ inner ].usage.led ) {
          case LIBUSB_HID_USAGE_PAGE_LED_NUMBER_LOCK:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Number lock led detected\r\n")
            #endif
            device->led_field[ 0 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.num_lock = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_CAPSLOCK:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Capslock lock led detected\r\n")
            #endif
            device->led_field[ 1 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.caps_lock = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_SCROLL_LOCK:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Scroll lock led detected\r\n")
            #endif
            device->led_field[ 2 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.scroll_lock = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_COMPOSE:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Compose led detected\r\n")
            #endif
            device->led_field[ 3 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.compose = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_KANA:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Kana led detected\r\n")
            #endif
            device->led_field[ 4 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.kana = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_POWER:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Power led detected\r\n")
            #endif
            device->led_field[ 5 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.power = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_SHIFT:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Shift led detected\r\n")
            #endif
            device->led_field[ 6 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.shift = true;
            break;
          case LIBUSB_HID_USAGE_PAGE_LED_MUTE:
            #if defined( KEYBOARD_ENABLE_DEBUG )
              EARLY_STARTUP_PRINT( "Mute led detected\r\n")
            #endif
            device->led_field[ 7 ] = &device->key_report->fields[ inner ];
            // set supported flag
            device->led.mute = true;
            break;
          default:
            break;
        }
      }
    }
    #if defined( KEYBOARD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Freeing report\r\n" )
    #endif
    // free report again
    hid_destroy_report( report );
  }
  #if defined( KEYBOARD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Allocate report buffer\r\n" )
  #endif
  // allocate report buffer
  device->buffer = malloc( KEYBOARD_REPORT_SIZE );
  if ( ! device->buffer ) {
    free( request );
    keyboard_destroy( device );
    err_response.status = -result;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
    return;
  }
  #if defined( KEYBOARD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Clear allocated buffer\r\n" )
  #endif
  // clear it out
  memset( device->buffer, 0, KEYBOARD_REPORT_SIZE );
  // finally append device to list
  keyboard_append( device );
  #if defined( KEYBOARD_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "endpoint_descriptor.endpoint_address.number = %"PRIu8"\r\n",
      endpoint_descriptor.endpoint_address.number );
    EARLY_STARTUP_PRINT( "endpoint_descriptor.interval = %"PRIu8"\r\n",
      endpoint_descriptor.interval );
  #endif
  // free request
  free( request );
  // return success
  memset( &err_response, 0, sizeof( err_response ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), nullptr, 0 );
}
