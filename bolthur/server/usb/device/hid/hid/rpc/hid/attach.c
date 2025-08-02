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
#include "hid.h"
#include "../../handler.h"
#include "../../rpc.h"
#include "../../global.h"
#include "../../../../../../libusbd.h"
#include "../../../../../../../library/usb/usb.h"

/**
 * @fn int hid_set_protocol(uint32_t, uint16_t, uint8_t)
 * @brief Set hid protocol
 * @param device_number
 * @param interface
 * @param protocol
 * @return
 */
static int hid_set_protocol(
  const uint32_t device_number,
  const uint16_t interface,
  const uint8_t protocol
) {
  uint32_t last_transfer;
  libusb_transfer_error_t error;
  // perform control message
  const int result = usb_control_message(
    device_number,
    LIBUSB_TRANSFER_CONTROL,
    LIBUSB_DIRECTION_OUT,
    NULL,
    0,
    &( libusb_device_request_t ){
      .request = LIBUSB_DEVICE_REQUEST_SET_PROTOCOL,
      .type = 0x21,
      .index = interface,
      .value = protocol,
      .length = 0,
    },
    10, /// FIXME: REPLACE WITH CONSTANT
    &error,
    &last_transfer
  );
  // handle error
  if ( 0 != result ) {
    return result;
  }
  // handle error
  if ( error != LIBUSB_TRANSFER_ERROR_NO_ERROR ) {
    return EIO;
  }
  // return success
  return 0;
}

/**
 * @fn void hid_enumerate_action_count_report(void*, libusb_hid_report_tag_t, uint32_t)
 * @brief Method to enumerate action count report
 * @param data
 * @param tag
 * @param value
 */
static void hid_enumerate_action_count_report(
  void* data,
  const libusb_hid_report_tag_t tag,
  [[maybe_unused]] uint32_t value
) {
  auto hid_report_action_count_t* report = ( hid_report_action_count_t* )data;
  // handle tag
  switch ( tag ) {
    case LIBUSB_HID_REPORT_TAG_MAIN_INPUT:
      if ( !report->input ) {
        report->count++;
        report->input = true;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_OUTPUT:
      if ( !report->output ) {
        report->count++;
        report->output = true;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_FEATURE:
      if ( !report->feature ) {
        report->count++;
        report->feature = true;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_ID:
      report->input = report->output = report->feature = false;
    default:
      break;
  }
}

/**
 * @fn void hid_enumerate_action_count_field_process(hid_report_field_t*, uint32_t, libusb_hid_report_type_t)
 * @brief Wrapper to enumerate action count field
 * @param field
 * @param value
 * @param type
 */
static void hid_enumerate_action_count_field_process(
  hid_report_field_t* field,
  uint32_t value,
  const libusb_hid_report_type_t type
) {
  hid_report_field_data_t* field_data = NULL;
  for ( size_t idx = 0; idx < field->current; idx++ ) {
    if (
      field->data[ idx ].report_id == field->report
      && field->data[ idx ].report_type == type
    ) {
      field_data = &field->data[ idx ];
      break;
    }
  }
  if ( ! field_data ) {
    field_data = &field->data[ field->current++ ];
    field_data->report_id = field->report;
    field_data->field_count = 0;
    field_data->report_type = type;
  }
  void* v = &value;
  if ( ( ( libusb_hid_main_item_t* )v )->variable ) {
    field_data->field_count += ( uint8_t )field->count;
  } else {
    field_data->field_count++;
  }
}

/**
 * @fn void hid_enumerate_action_count_field(void*, libusb_hid_report_tag_t, uint32_t)
 * @brief Method to enumerate action count fields
 * @param data
 * @param tag
 * @param value
 */
static void hid_enumerate_action_count_field(
  void* data,
  const libusb_hid_report_tag_t tag,
  const uint32_t value
) {
  auto hid_report_field_t* field = ( hid_report_field_t* )data;
  switch ( tag ) {
    case LIBUSB_HID_REPORT_TAG_MAIN_FEATURE:
      hid_enumerate_action_count_field_process( field, value, LIBUSB_HID_REPORT_TYPE_FEATURE );
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_OUTPUT:
      hid_enumerate_action_count_field_process( field, value, LIBUSB_HID_REPORT_TYPE_OUTPUT );
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_INPUT:
      hid_enumerate_action_count_field_process( field, value, LIBUSB_HID_REPORT_TYPE_INPUT );
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_COUNT:
      field->count = value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_ID:
      field->report = ( uint8_t )value;
      break;
    default:
      break;
  }
}

/**
 * @fn void hid_enumerate_action_add_field_process(hid_field_t*, uint32_t, libusb_hid_report_type_t)
 * @brief Wrapper to process enumerate action add field
 * @param field
 * @param value
 * @param type
 */
static void hid_enumerate_action_add_field_process(
  hid_field_t* field,
  const uint32_t value,
  const libusb_hid_report_type_t type
) {
  // try to find report from result
  libusb_hid_parser_report_t* report = NULL;
  for ( uint32_t idx = 0; idx < field->result->report_count; idx++ ) {
    EARLY_STARTUP_PRINT( "field->result->report[ %"PRIu32" ]->id = %"PRIu8"\r\n",
      idx, field->result->report[ idx ]->id )
    EARLY_STARTUP_PRINT( "field->result->report[ %"PRIu32" ]->type = %d\r\n",
      idx, field->result->report[ idx ]->type )
    if (
      field->result->report[ idx ]->id == field->report
      && field->result->report[ idx ]->type == type
    ) {
      report = field->result->report[ idx ];
      break;
    }
  }
  // handle no report found
  if ( ! report ) {
    STARTUP_PRINT( "Report not found for %"PRIu8" / %d\r\n",
      field->report, type )
    return;
  }
  // loop while field count is greater than 0
  while ( field->count > 0 ) {
    // extract val
    uint32_t val;
    memcpy( &val, field->usage, sizeof( val ) );
    // handle first iteration
    if ( val == 0xffffffff ) {
      field->usage++;
    }
    memcpy( &report->fields[ report->field_count ].attribute, &value, sizeof( uint32_t ) );
    report->fields[ report->field_count ].count = ( uint8_t)( report->fields[ report->field_count ].attribute.variable ? 1 : field->count );
    report->fields[ report->field_count ].logical_maximum = field->logical_maximum;
    report->fields[ report->field_count ].logical_minimum = field->logical_minimum;
    report->fields[ report->field_count ].offset = report->report_length;
    report->fields[ report->field_count ].physical_maximum = field->physical_maximum;
    report->fields[ report->field_count ].physical_minimum = field->physical_minimum;
    memcpy( &report->fields[ report->field_count ].physical_usage, &field->physical, sizeof( field->physical ) );
    report->fields[ report->field_count ].size = ( uint8_t )field->size;
    memcpy( &report->fields[ report->field_count ].unit, &field->unit, sizeof( field->unit ) );
    report->fields[ report->field_count ].unit_exponent = field->unit_exponent;
    if ( ( uint16_t )field->usage->page == LIBUSB_HID_USAGE_PAGE_USAGE_PAGE ) {
      memcpy( &report->fields[ report->field_count ].usage, &field->usage[ -1 ], sizeof( uint32_t ) );
      if (
        field->usage->desktop == field->usage[ -1 ].desktop
        || ! report->fields[ report->field_count ].attribute.variable
      ) {
        field->usage -= 2;
      } else {
        field->usage[ -1 ].desktop++;
      }
    } else {
      memcpy( &report->fields[ report->field_count ].usage, field->usage--, sizeof( uint32_t ) );
    }
    if (report->fields[ report->field_count ].attribute.variable ) {
      field->count--;
      report->report_length += report->fields[ report->field_count ].size;
      report->fields[ report->field_count ].value.u32 = 0;
    } else {
      field->count = 0;
      report->report_length += ( uint8_t )( report->fields[ report->field_count ].size * report->fields[ report->field_count ].count );
      report->fields[ report->field_count ].value.ptr = malloc(
        ( size_t )( report->fields[ report->field_count ].size * report->fields[ report->field_count ].count / 8 ) );
    }
    report->field_count++;
  }
  // set field usage 1 to 0
  constexpr uint32_t val = 0;
  memcpy( &field->usage[ 1 ], &val, sizeof( uint32_t ) );
}

/**
 * @fn void hid_enumerate_action_add_field(void*, libusb_hid_report_tag_t, uint32_t )
 * @brief Method to enumerate action add fields
 * @param data
 * @param tag
 * @param value
 */
static void hid_enumerate_action_add_field(
  void* data,
  const libusb_hid_report_tag_t tag,
  const uint32_t value
) {
  auto hid_field_t* field = ( hid_field_t* )data;

  switch ( tag ) {
    case LIBUSB_HID_REPORT_TAG_MAIN_FEATURE:
      hid_enumerate_action_add_field_process( field, value, LIBUSB_HID_REPORT_TYPE_FEATURE );
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_OUTPUT:
      hid_enumerate_action_add_field_process( field, value, LIBUSB_HID_REPORT_TYPE_OUTPUT );
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_INPUT:
      hid_enumerate_action_add_field_process( field, value, LIBUSB_HID_REPORT_TYPE_INPUT );
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_COLLECTION:
      uint32_t val;
      memcpy( &val, field->usage, sizeof( uint32_t ) );
      if ( 0xffffffff == val ) {
        field->usage++;
      }
      switch ( ( libusb_hid_main_collection_t )value ) {
        case LIBUSB_HID_MAIN_COLLECTION_APPLICATION:
          memcpy( &field->result->application, field->usage, sizeof( uint32_t ) );
          break;
        case LIBUSB_HID_MAIN_COLLECTION_PHYSICAL:
          memcpy( &field->physical, field->usage, sizeof( uint32_t ) );
          break;
        default:
          break;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_MAIN_END_COLLECTION:
      switch ( ( libusb_hid_main_collection_t )value ) {
        case LIBUSB_HID_MAIN_COLLECTION_PHYSICAL:
          memset( &field->physical, 0, sizeof( uint32_t ) );
          break;
        default:
          break;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_USAGE_PAGE:
      field->page = ( libusb_hid_usage_page_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_LOGICAL_MINIMUM:
      field->logical_minimum = ( int32_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_LOGICAL_MAXIMUM:
      field->logical_maximum = ( int32_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_PHYSICAL_MINIMUM:
      field->physical_minimum = ( int32_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_PHYSICAL_MAXIMUM:
      field->physical_maximum = ( int32_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_UNIT_EXPONENT:
      field->unit_exponent = ( int32_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_UNIT:
      memcpy( &field->unit, &value, sizeof( uint32_t ) );
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_SIZE:
      field->size = value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_ID:
      field->report = ( uint8_t )value;
      break;
    case LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_COUNT:
      field->count = value;
      break;
    case LIBUSB_HID_REPORT_TAG_LOCAL_USAGE:
      field->usage++;
      if ( value & 0xffff0000 ) {
        memcpy( &field->usage, &value, sizeof( uint32_t ) );
      } else {
        field->usage->desktop = ( libusb_hid_usage_page_desktop_t )value;
        field->usage->page = field->page;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_LOCAL_USAGE_MINIMUM:
      field->usage++;
      if ( value & 0xffff0000 ) {
        memcpy( &field->usage, &value, sizeof( uint32_t ) );
      } else {
        field->usage->desktop = ( libusb_hid_usage_page_desktop_t )value;
        field->usage->page = field->page;
      }
      break;
    case LIBUSB_HID_REPORT_TAG_LOCAL_USAGE_MAXIMUM:
      field->usage++;
      field->usage->desktop = ( libusb_hid_usage_page_desktop_t )value;
      field->usage->page = LIBUSB_HID_USAGE_PAGE_USAGE_PAGE;
      break;
    default:
      break;
  }
}

/**
 * @fn void hid_enumerate_report(void*, const size_t, hid_report_action_t, void*)
 * @brief Method to enumerate report with callback
 * @param descriptor
 * @param length
 * @param action
 * @param data
 */
static void hid_enumerate_report(
  void* descriptor,
  const size_t length,
  const hid_report_action_t action,
  void* data
) {
  auto libusb_hid_report_item_t* item = ( libusb_hid_report_item_t* )descriptor;
  libusb_hid_report_item_t* current = NULL;
  size_t parsed_length = 0;
  size_t current_index;
  size_t current_length;
  uint32_t value;
  libusb_hid_report_tag_t tag;

  while ( parsed_length < length ) {
    if ( ! current ) {
      current = item;
      current_index = 0;
      current_length = 1 << ( current->size - 1 );
      value = 0;
      tag = current->tag;
      if ( current_length == 0 ) {
        current = NULL;
      }
    } else {
      if ( current->tag == LIBUSB_HID_REPORT_TAG_LONG && current_index < 2 ) {
        if ( current_index == 0 ) {
          current_length += *(uint8_t*)item;
        } else {
          tag |= ( uint16_t )*( uint8_t*)item << 8;
        }
      } else {
        value |= ( uint32_t )( *( uint8_t* )item << ( 8 * current_index ) );
      }
      if ( ++current_index == current_length ) {
        current = NULL;
      }
    }

    if ( ! current ) {
      if ( ( tag & 0x3 ) == 0x1 ) {
        if ( current_length == 1 && ( value & 0x80 ) ) {
          value |= 0xffffff00;
        } else if ( current_length == 2 && ( value & 0x8000 ) ) {
          value |= 0xffff0000;
        }
      }
      action( data, tag, value );
    }

    item++;
    parsed_length++;
  }
}

/**
 * @fn int hid_parse_report_descriptor(libusb_hid_device_t*, void*, size_t)
 * @brief Method to parse report descriptor
 * @param device
 * @param descriptor
 * @param length
 * @return
 */
static int hid_parse_report_descriptor(
  libusb_hid_device_t* device,
  void* descriptor,
  const size_t length
) {
  hid_report_action_count_t header = {
    .count = 0,
    .indent = 0,
    .input = false,
    .output = false,
    .feature = false,
  };
  hid_report_field_t* report_field = NULL;
  hid_field_t* field = NULL;

  // enumerate action count
  hid_enumerate_report( descriptor, length, hid_enumerate_action_count_report, &header );
  STARTUP_PRINT( "Found %"PRIu8" reports!\r\n", header.count )
  // allocate space
  libusb_hid_parser_result_t* result = malloc(
    sizeof( libusb_hid_parser_result_t ) + sizeof( libusb_hid_parser_report_t* ) * header.count );
  if ( ! result ) {
    STARTUP_PRINT( "Unable to allocate memory for parser result\r\n" )
    return ENOMEM;
  }
  // clear out
  memset( result, 0, sizeof( libusb_hid_parser_result_t ) + sizeof( libusb_hid_parser_report_t* ) * header.count );
  // allocate space for report field
  report_field = malloc( sizeof( hid_report_field_t ) + sizeof( hid_report_field_data_t ) * header.count );
  if ( ! report_field ) {
    STARTUP_PRINT( "Unable to allocate memory for report field\r\n" )
    free( result );
    return ENOMEM;
  }
  // clear out
  memset( report_field, 0, sizeof( hid_report_field_t ) + sizeof( hid_report_field_data_t ) * header.count );
  // prepare data
  result->report_count = header.count;

  // enumerate report fields
  hid_enumerate_report( descriptor, length, hid_enumerate_action_count_field, report_field );
  for ( size_t idx = 0; idx < header.count; idx++ ) {
    // allocate space for report
    result->report[ idx ] = malloc(
      sizeof( libusb_hid_parser_report_t ) + sizeof( libusb_hid_parser_fields_t ) * report_field->data[ idx ].field_count );
    // handle error
    if ( ! result->report[ idx ] ) {
      STARTUP_PRINT( "Unable to allocate space for report\r\n" )
      // free possible reports
      for ( size_t free_idx = 0; free_idx < header.count; free_idx++ ) {
        if ( result->report[ free_idx ] ) {
          free( result->report[ free_idx ] );
        }
      }
      // free report field and result
      free( report_field );
      free( result );
      return ENOMEM;
    }
    // fill report
    result->report[ idx ]->index = ( uint8_t )idx;
    result->report[ idx ]->field_count = 0;
    result->report[ idx ]->id = report_field->data[ idx ].report_id;
    result->report[ idx ]->type = report_field->data[ idx ].report_type;
    result->report[ idx ]->report_length = 0;
    result->report[ idx ]->report_buffer = NULL;
  }
  // free again report fields
  free( report_field );

  // allocate space for field
  field = malloc( sizeof( hid_field_t ) );
  if ( ! field ) {
    STARTUP_PRINT( "Unable to allocate space for report\r\n" )
    // free possible reports
    for ( size_t free_idx = 0; free_idx < header.count; free_idx++ ) {
      if ( result->report[ free_idx ] ) {
        free( result->report[ free_idx ] );
      }
    }
    free( result );
    return ENOMEM;
  }
  // clear out
  memset( field, 0, sizeof( hid_field_t ) );
  // set fields usage
  field->usage = calloc(16, sizeof( libusb_hid_full_usage_t* ) );
  if ( ! field->usage ) {
    STARTUP_PRINT( "Unable to allocate space for report\r\n" )
    // free possible reports
    for ( size_t free_idx = 0; free_idx < header.count; free_idx++ ) {
      if ( result->report[ free_idx ] ) {
        free( result->report[ free_idx ] );
      }
    }
    free( field );
    free( result );
    return ENOMEM;
  }
  // populate field
  constexpr uint32_t val = 0xffffffff;
  memcpy( field->usage, &val, sizeof( libusb_hid_full_usage_t ) );
  field->result = result;
  // cache usage field
  void* usage = field->usage;
  // enumerate fields
  hid_enumerate_report( descriptor, length, hid_enumerate_action_add_field, field );

  // populate result to device
  device->parser_result = result;

  // free usage field and field again
  free( usage );
  free( field );

  // return success
  return 0;
}

/**
 * @fn void rpc_hid_attach(size_t, pid_t, size_t, size_t)
 * @brief Register rpc handler attach
 * @param type message type
 * @param origin origin of the message
 * @param data_info data id
 * @param response_info response info
 */
void rpc_hid_attach(
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

  // get interface information
  libusb_interface_descriptor_t interface_descriptor;
  int result = usb_get_interface(
    message->device_number, message->interface_number, &interface_descriptor );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get interface data\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // get endpoint information
  libusb_endpoint_descriptor_t endpoint_descriptor;
  result = usb_get_endpoint(
    message->device_number, message->interface_number, 0, &endpoint_descriptor );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get endpoint information\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // validate class
  if ( interface_descriptor.class != LIBUSB_INTERFACE_CLASS_HID ) {
    STARTUP_PRINT( "Invalid interfacae class\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // validate interface endpoint
  if ( interface_descriptor.endpoint_count < 1 ) {
    STARTUP_PRINT( "Invalid hid device with fewer than one endpoint\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // validate endpoint
  if (
    endpoint_descriptor.endpoint_address.direction != LIBUSB_DIRECTION_IN
    || endpoint_descriptor.attributes.transfer != LIBUSB_TRANSFER_INTERRUPT
  ) {
    STARTUP_PRINT( "Invalid hid device with unusual endpoints\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // fetch device status
  libusb_device_status_t status;
  result = usb_get_status( message->device_number, &status );
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get device status\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // ensure it's configured
  if ( status != LIBUSB_DEVICE_STATUS_CONFIGURED ) {
    STARTUP_PRINT( "Device not configured\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // check for boot device
  if ( interface_descriptor.subclass == 1 ) {
    if ( interface_descriptor.protocol == 1 ) {
      STARTUP_PRINT( "Boot keyboard detected\r\n" )
    } else if ( interface_descriptor.protocol == 2 ) {
      STARTUP_PRINT( "Boot mouse detected\r\n" )
    } else {
      STARTUP_PRINT( "Unknown boot device detected\r\n" )
    }

    // switch protocol from boot to report mode
    STARTUP_PRINT( "Reverting from boot to normal hid mode\r\n" )
    result = hid_set_protocol(
      message->device_number, ( uint16_t )message->interface_number, 1 );
    if ( 0 != result ) {
      STARTUP_PRINT( "Could not revert to report mode\r\n" )
      _syscall_rpc_cleanup();
      free( request );
      return;
    }
  }

  // fetch configuration
  libusb_descriptor_header_t* header;
  result = usb_get_configuration( message->device_number, ( void** )&header );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to fetch usb device configuration: %s\r\n",
      strerror( result ) )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // find descriptor of hid
  libusb_descriptor_header_t* original_header = header;
  libusb_hid_descriptor_t* descriptor = NULL;
  uint32_t interface_number = message->interface_number + 1;
  do {
    // handle end reached
    if ( ! header->descriptor_length ) {
      break;
    }
    // switch descriptor type
    switch ( header->descriptor_type ) {
      case LIBUSB_DESCRIPTOR_INTERFACE:
        interface_number = ( ( libusb_interface_descriptor_t* )header )->number;
        break;
      case LIBUSB_DESCRIPTOR_HID:
        if ( interface_number == message->interface_number ) {
          descriptor = ( libusb_hid_descriptor_t* )header;
        }
        break;
      default:
        break;
    }
    // some debug output
    STARTUP_PRINT( "Descriptor %d with length %"PRIu8". Interface: %"PRIu32"\r\n",
      header->descriptor_type, header->descriptor_length, interface_number )
    // handle descriptor found
    if ( descriptor ) {
      break;
    }
    // go to next header
    header = ( libusb_descriptor_header_t* )( ( uint8_t* )header + header->descriptor_length );
  } while ( true );
  // validate hid descriptor
  if ( ! descriptor ) {
    STARTUP_PRINT( "No hid descriptor in %s with interface %"PRIu32". Cannot be a hid device\r\n",
      usb_get_description(message->device_number), message->interface_number + 1 )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // check for hid version
  if ( descriptor->hid_version > 0x111 ) {
    STARTUP_PRINT( "Unsupported hid version: %"PRIx16".%"PRIx16"\r\n",
      descriptor->hid_version >> 8, descriptor->hid_version & 0xff )
    _syscall_rpc_cleanup();
    free( request );
    return;
  }
  // some debug output
  STARTUP_PRINT( "Detected hid device: %"PRIx16".%"PRIx16"\r\n",
    descriptor->hid_version >> 8, descriptor->hid_version & 0xff )
  // allocate hid device
  libusb_hid_device_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    STARTUP_PRINT( "Could not allocate device structure\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    free( original_header );
    return;
  }
  // clear out stuff
  memset( device, 0, sizeof( *device ) );
  // populate device
  device->device_number = message->device_number;
  // allocate report descriptor
  void* report_descriptor = malloc( descriptor->optional[ 0 ].length );
  if ( ! report_descriptor ) {
    STARTUP_PRINT( "Could not allocate reportDescriptor\r\n" )
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    return;
  }
  // clear out stuff
  memset( report_descriptor, 0, descriptor->optional[ 0 ].length );
  // pure in descriptor
  device->descriptor = descriptor;
  // request descriptor
  result = usb_get_descriptor(
    message->device_number,
    LIBUSB_DESCRIPTOR_HID_REPORT,
    0,
    ( uint16_t )message->interface_number,
    report_descriptor,
    descriptor->optional[ 0 ].length,
    descriptor->optional[ 0 ].length,
    1
  );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to get hid report descriptor: %s\r\n",
      strerror( result ) )
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }
  // parse report descriptor
  result = hid_parse_report_descriptor(
    device, report_descriptor, descriptor->optional[ 0 ].length );
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to parse hid report descriptor: %s\r\n",
      strerror( result ) )
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }

  // populate device
  device->parser_result->interface = ( uint8_t )message->interface_number;
  // try to attach
  result = handler_call_attach(
    device->parser_result->application.desktop,
    device,
    message->device_number,
    message->interface_number
  );
  // handle error
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to call attach device: %s\r\n",
      strerror( result ) );
    _syscall_rpc_cleanup();
    free( request );
    hid_destroy_device( device );
    free( original_header );
    free( report_descriptor );
    return;
  }
  // append to handled devices
  hid_append( device );
  // free up unnecessary stuff
  free( request );
  free( original_header );
  free( report_descriptor );
  // call rpc cleanup since there is no return
  _syscall_rpc_cleanup();
}
