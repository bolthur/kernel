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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include "hid.h"
#include "../../../../libusb.h"
#include "../../../../libusbd.h"
#include "../../../../../library/usb/usb.h"

libusb_hid_device_t* hid_head = NULL;

/**
 * @fn void hid_destroy_device(libusb_hid_device_t*)
 * @brief Helper to destroy hid device
 * @param device
 */
void hid_destroy_device( libusb_hid_device_t* device ) {
  // handle invalid device
  if ( ! device ) {
    return;
  }
  // remove from list
  if ( device->prev ) {
    device->prev->next = device->next;
  }
  if ( device->next ) {
    device->next->prev = device->prev;
  }
  // handle parser result set
  if ( device->parser_result ) {
    // free possible reports
    for ( size_t idx = 0; idx < device->parser_result->report_count; idx++ ) {
      // handle report allocated
      if ( device->parser_result->report[ idx ] ) {
        // free report
        free( device->parser_result->report[ idx ] );
      }
    }
    // free parser result
    free( device->parser_result );
  }
  // free device
  free( device );
}

/**
 * @fn int hid_set_protocol(uint32_t, uint16_t, uint8_t)
 * @brief Set hid protocol
 * @param device_number
 * @param interface
 * @param protocol
 * @return
 */
int hid_set_protocol(
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
    USB_TIMEOUT_VALUE,
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
void hid_enumerate_action_count_report(
  void* data,
  const libusb_hid_report_tag_t tag,
  [[maybe_unused]] uint32_t value
) {
  auto const report = ( hid_report_action_count_t* )data;
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
void hid_enumerate_action_count_field_process(
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
void hid_enumerate_action_count_field(
  void* data,
  const libusb_hid_report_tag_t tag,
  const uint32_t value
) {
  auto const field = ( hid_report_field_t* )data;
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
void hid_enumerate_action_add_field_process(
  hid_field_t* field,
  const uint32_t value,
  const libusb_hid_report_type_t type
) {
  // try to find report from result
  libusb_hid_parser_report_t* report = NULL;
  for ( uint32_t idx = 0; idx < field->result->report_count; idx++ ) {
    STARTUP_PRINT( "field->result->report[ %"PRIu32" ]->id = %"PRIu8"\r\n",
      idx, field->result->report[ idx ]->id )
    STARTUP_PRINT( "field->result->report[ %"PRIu32" ]->type = %d\r\n",
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
void hid_enumerate_action_add_field(
  void* data,
  const libusb_hid_report_tag_t tag,
  const uint32_t value
) {
  auto const field = ( hid_field_t* )data;

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
void hid_enumerate_report(
  void* descriptor,
  const size_t length,
  const hid_report_action_t action,
  void* data
) {
  auto item = ( libusb_hid_report_item_t* )descriptor;
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
int hid_parse_report_descriptor(
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
    result->report[ idx ]->fields_length = report_field->data[ idx ].field_count;
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
 * @fn void hid_append(libusb_hid_device_t*)
 * @brief Append hid to handled devices
 * @param hid
 */
void hid_append( libusb_hid_device_t* hid ) {
  // loop to last one
  libusb_hid_device_t* current = hid_head;
  libusb_hid_device_t* found = NULL;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    hid_head = hid;
    hid->prev = NULL;
    hid->next = NULL;
    return;
  }
  // attach to list
  found->next = hid;
  hid->prev = found;
}

/**
 * @fn int hid_get(uint32_t, libusb_hid_device_t**)
 * @brief Function to get hid device
 * @param device_number
 * @param hid
 * @return
 */
int hid_get( const uint32_t device_number, libusb_hid_device_t** hid ) {
  // setup current
  libusb_hid_device_t* current = hid_head;
  // loop while there is an entry
  while ( current ) {
    // handle match
    if ( current->device_number == device_number ) {
      // set pointer
      *hid = current;
      // return success
      return 0;
    }
    // go to next
    current = current->next;
  }
  // return error
  return ENOENT;
}
