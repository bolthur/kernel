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

#ifndef _LIBUSB_H
#define _LIBUSB_H

#include <stdint.h>
#include <sys/types.h>

// internal usb declarations

#define MAX_CHILDREN_PER_DEVICE 10
#define MAX_INTERFACES_PER_DEVICE 8
#define MAX_ENDPOINTS_PER_DEVICE 16

// disable a bunch of warnings necessary to build packed structures
// for usb communication
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"

typedef enum {
  LIBUSB_DESCRIPTOR_DEVICE = 1,
  LIBUSB_DESCRIPTOR_CONFIGURATION = 2,
  LIBUSB_DESCRIPTOR_STRING = 3,
  LIBUSB_DESCRIPTOR_INTERFACE = 4,
  LIBUSB_DESCRIPTOR_ENDPOINT = 5,
  LIBUSB_DESCRIPTOR_DEVICE_QUALIFIER = 6,
  LIBUSB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION = 7,
  LIBUSB_DESCRIPTOR_INTERFACE_POWER = 8,
  LIBUSB_DESCRIPTOR_HID = 33,
  LIBUSB_DESCRIPTOR_HID_REPORT = 34,
  LIBUSB_DESCRIPTOR_HID_PHYSICAL = 35,
  LIBUSB_DESCRIPTOR_HUB = 41,
} libusb_descriptor_type_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
} libusb_descriptor_header_t;

typedef enum {
  LIBUSB_DEVICE_CLASS_IN_INTERFACE = 0x00,
  LIBUSB_DEVICE_CLASS_COMMUNICATIONS = 0x02,
  LIBUSB_DEVICE_CLASS_HUB = 0x09,
  LIBUSB_DEVICE_CLASS_DIAGNOSTIC = 0xdc,
  LIBUSB_DEVICE_CLASS_MISCELLANEOUS = 0xef,
  LIBUSB_DEVICE_CLASS_VENDOR_SPECIFIC = 0xff,
} libusb_device_class_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint16_t usb_version;
  libusb_device_class_t class : 8;
  uint8_t subclass;
  uint8_t protocol;
  uint8_t max_packet_size0;
  uint16_t vendor_id;
  uint16_t product_id;
  uint16_t version;
  uint8_t manufacturer;
  uint8_t product;
  uint8_t serial_number;
  uint8_t configuration_count;
} libusb_device_descriptor_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint16_t usb_version;
  libusb_device_class_t class : 8;
  uint8_t subclass;
  uint8_t protocol;
  uint8_t max_packet_size0;
  uint8_t configuration_count;
  uint8_t _reserved;
} libusb_device_qualifier_descriptor_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint16_t total_length;
  uint8_t interface_count;
  uint8_t configuration_value;
  uint8_t string_index;
  struct __packed {
    uint8_t reserved0 : 5;
    bool remote_wakeup : 1;
    bool self_powered : 1;
    uint8_t reserved1 : 1;
  } attributes;
  uint8_t maximum_power;
} libusb_configuration_descriptor_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint16_t total_length;
  uint8_t interface_count;
  uint8_t configuration_value;
  uint8_t interface_number;
  struct __packed {
    uint8_t reserved0 : 5;
    bool remote_wakeup : 1;
    bool self_powered : 1;
    uint8_t reserved1 : 1;
  } attributes;
  uint8_t maximum_power;
} libusb_other_speed_configuration_descriptor_t;

typedef enum {
  LIBUSB_INTERFACE_CLASS_RESERVED = 0x00,
  LIBUSB_INTERFACE_CLASS_AUDIO = 0x01,
  LIBUSB_INTERFACE_CLASS_COMMUNICATIONS = 0x02,
  LIBUSB_INTERFACE_CLASS_HID = 0x03,
  LIBUSB_INTERFACE_CLASS_PHYSICAL = 0x05,
  LIBUSB_INTERFACE_CLASS_IMAGE = 0x06,
  LIBUSB_INTERFACE_CLASS_PRINTER = 0x07,
  LIBUSB_INTERFACE_CLASS_MASS_STORAGE = 0x08,
  LIBUSB_INTERFACE_CLASS_HUB = 0x09,
  LIBUSB_INTERFACE_CLASS_CDC_DATA = 0x0a,
  LIBUSB_INTERFACE_CLASS_SMART_CARD = 0x0b,
  LIBUSB_INTERFACE_CLASS_CONTENT_SECURITY = 0x0d,
  LIBUSB_INTERFACE_CLASS_VIDEO = 0x0e,
  LIBUSB_INTERFACE_CLASS_PERSONAL_HEALTHCARE = 0x0f,
  LIBUSB_INTERFACE_CLASS_AUDIO_VIDEO = 0x10,
  LIBUSB_INTERFACE_CLASS_DIAGNOSTIC_DEVICE = 0xdc,
  LIBUSB_INTERFACE_CLASS_WIRELESS_CONTROLLER = 0xe0,
  LIBUSB_INTERFACE_CLASS_MISCELLANEOUS = 0xef,
  LIBUSB_INTERFACE_CLASS_APPLICATION_SPECIFIC = 0xfe,
  LIBUSB_INTERFACE_CLASS_VENDOR_SPECIFIC = 0xff,
} libusb_interface_class_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint8_t number;
  uint8_t alternate_setting;
  uint8_t endpoint_count;
  libusb_interface_class_t class : 8;
  uint8_t subclass;
  uint8_t protocol;
  uint8_t string_index;
} libusb_interface_descriptor_t;

typedef enum {
  LIBUSB_DIRECTION_HOST_TO_DEVICE = 0,
  LIBUSB_DIRECTION_OUT = 0,
  LIBUSB_DIRECTION_DEVICE_TO_HOST = 1,
  LIBUSB_DIRECTION_IN = 1,
} libusb_direction_t;

typedef enum {
  LIBUSB_TRANSFER_CONTROL = 0,
  LIBUSB_TRANSFER_ISOCHRONOUS = 1,
  LIBUSB_TRANSFER_BULK = 2,
  LIBUSB_TRANSFER_INTERRUPT = 3,
} libusb_transfer_t;

typedef enum {
  LIBUSB_SYNCHRONIZATION_NO_SYNCHRONIZATION = 0,
  LIBUSB_SYNCHRONIZATION_ASYNCHRONOUS = 1,
  LIBUSB_SYNCHRONIZATION_ADAPTIVE = 2,
  LIBUSB_SYNCHRONIZATION_SYNCHRONOUS = 3,
} libusb_synchronization_t;

typedef enum {
  LIBUSB_USAGE_DATA = 0,
  LIBUSB_USAGE_FEEDBACK = 1,
  LIBUSB_USAGE_IMPLICIT_FEEDBACK_DATA = 2,
} libusb_usage_t;

typedef enum {
  LIBUSB_TRANSACTIONS_NONE = 0,
  LIBUSB_TRANSACTIONS_EXTRA1 = 1,
  LIBUSB_TRANSACTIONS_EXTRA2 = 2,
} libusb_transactions_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  struct __packed {
    uint8_t number : 4;
    uint8_t reserved : 3;
    libusb_direction_t direction : 1;
  } endpoint_address;
  struct __packed {
    libusb_transfer_t transfer : 2;
    libusb_synchronization_t synchronization : 2;
    libusb_usage_t usage : 2;
    uint8_t reserved : 2;
  } attributes;
  struct __packed {
    uint16_t max_size : 11;
    libusb_transactions_t transactions : 2;
    uint8_t reserved : 3;
  } packet;
  uint8_t interval;
} libusb_endpoint_descriptor_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint16_t data[];
} libusb_string_descriptor_t;

typedef enum {
  LIBUSB_PACKET_SIZE_BITS_8,
  LIBUSB_PACKET_SIZE_BITS_16,
  LIBUSB_PACKET_SIZE_BITS_32,
  LIBUSB_PACKET_SIZE_BITS_64,
} libusb_packet_size_t;

/**
 * @brief Get packet size from number
 * @param size
 * @return
 */
[[maybe_unused]] static libusb_packet_size_t usb_packet_size_from_number(
  const uint32_t size
) {
  if (size <= 8) {
    return LIBUSB_PACKET_SIZE_BITS_8;
  }
  if (size <= 16) {
    return LIBUSB_PACKET_SIZE_BITS_16;
  }
  if (size <= 32) {
    return LIBUSB_PACKET_SIZE_BITS_32;
  }
  return LIBUSB_PACKET_SIZE_BITS_64;
}

/**
 * @brief Transform size to number
 * @param size
 * @return
 */
[[maybe_unused]] static uint32_t usb_number_from_packet_size(
  const libusb_packet_size_t size
) {
  switch ( size ) {
    case LIBUSB_PACKET_SIZE_BITS_8: return 8;
    case LIBUSB_PACKET_SIZE_BITS_16: return 16;
    case LIBUSB_PACKET_SIZE_BITS_32: return 32;
    default: return 64;
  }
}

typedef enum {
  LIBUSB_SPEED_HIGH = 0,
  LIBUSB_SPEED_FULL = 1,
  LIBUSB_SPEED_LOW = 2,
} libusb_speed_t;

/**
 * @brief Small static function to turn speed into string for printing purposes
 * @param speed speed to translate
 * @return translated speed
 */
[[maybe_unused]] static char* usb_speed_to_string( const libusb_speed_t speed ) {
  if ( LIBUSB_SPEED_HIGH == speed ) {
    return "480 Mb/s";
  }
  if ( LIBUSB_SPEED_LOW == speed ) {
    return "1.5 Mb/s";
  }
  if ( LIBUSB_SPEED_FULL == speed ) {
    return "12 Mb/s";
  }
  return "Unknown Mb/s";
}

typedef struct __packed {
  libusb_packet_size_t max_size : 2;
  libusb_speed_t speed : 2;
  uint8_t end_point : 4;
  uint8_t device : 8;
  libusb_transfer_t type : 2;
  libusb_direction_t direction : 1;
  uint16_t reserved : 13;
} libusb_pipe_address_t;

typedef enum {
  // usb requests
  LIBUSB_DEVICE_REQUEST_GET_STATUS = 0,
  LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE = 1,
  LIBUSB_DEVICE_REQUEST_SET_FEATURE = 3,
  LIBUSB_DEVICE_REQUEST_SET_ADDRESS = 5,
  LIBUSB_DEVICE_REQUEST_GET_DESCRIPTOR = 6,
  LIBUSB_DEVICE_REQUEST_SET_DESCRIPTOR = 7,
  LIBUSB_DEVICE_REQUEST_GET_CONFIGURATION = 8,
  LIBUSB_DEVICE_REQUEST_SET_CONFIGURATION = 9,
  LIBUSB_DEVICE_REQUEST_GET_INTERFACE = 10,
  LIBUSB_DEVICE_REQUEST_SET_INTERFACE = 11,
  LIBUSB_DEVICE_REQUEST_SYNCH_FRAME = 12,
  // hid requests
  LIBUSB_DEVICE_REQUEST_GET_REPORT = 1,
  LIBUSB_DEVICE_REQUEST_GET_IDLE = 2,
  LIBUSB_DEVICE_REQUEST_GET_PROTOCOL = 3,
  LIBUSB_DEVICE_REQUEST_SET_REPORT = 9,
  LIBUSB_DEVICE_REQUEST_SET_IDLE = 10,
  LIBUSB_DEVICE_REQUEST_SET_PROTOCOL = 11,
} libusb_device_request_enum_t;

typedef struct __packed {
  uint8_t type;
  libusb_device_request_enum_t request : 8;
  uint16_t value;
  uint16_t index;
  uint16_t length;
} libusb_device_request_t;

typedef enum {
  LIBUSB_DEVICE_STATUS_ATTACHED = 0,
  LIBUSB_DEVICE_STATUS_POWERED = 1,
  LIBUSB_DEVICE_STATUS_DEFAULT = 2,
  LIBUSB_DEVICE_STATUS_ADDRESSED = 3,
  LIBUSB_DEVICE_STATUS_CONFIGURED = 4,
} libusb_device_status_t;

typedef enum {
  LIBUSB_TRANSFER_ERROR_NO_ERROR = 0,
  LIBUSB_TRANSFER_ERROR_STALL = 1 << 1,
  LIBUSB_TRANSFER_ERROR_BUFFER_ERROR = 1 << 2,
  LIBUSB_TRANSFER_ERROR_BABBLE = 1 << 3,
  LIBUSB_TRANSFER_ERROR_NO_ACKNOWLEDGE = 1 << 4,
  LIBUSB_TRANSFER_ERROR_CRC_ERROR = 1 << 5,
  LIBUSB_TRANSFER_ERROR_BIT_ERROR = 1 << 6,
  LIBUSB_TRANSFER_ERROR_CONNECTION_ERROR = 1 << 7,
  LIBUSB_TRANSFER_ERROR_AHB_ERROR = 1 << 8,
  LIBUSB_TRANSFER_ERROR_NOT_YET_ERROR = 1 << 9,
  LIBUSB_TRANSFER_ERROR_PROCESSING = 1 << 10,
} libusb_transfer_error_t;

typedef struct {
  uint32_t device_driver;
  uint32_t data_size;
} libusb_driver_data_header;

typedef struct libusb_device libusb_device_t;
typedef struct libusb_device {
  uint32_t number;

  libusb_speed_t speed;
  libusb_device_status_t status;
  uint8_t configuration_index;
  uint8_t port_number;
  libusb_transfer_error_t error __aligned( 4 );

  // processes responsible for generic detach and deallocate
  pid_t device_detached_handler;
  pid_t device_deallocate_handler;
  // processes responsible for hub actions check for change, child detached
  // child reset and check connection
  pid_t device_check_for_change_handler;
  pid_t device_child_detached_handler;
  pid_t device_child_reset_handler;
  pid_t device_check_connection_handler;

  /// FIXME: REPLACE POINTER TO FUNCTIONS BY PID CALLS
  /** Handler for detaching the device. The device driver should not issue further requests to the device. */
  void ( *device_detached )( libusb_device_t* device ) __aligned( 4 );
  /** Handler for deallocation of the device. All memory in use by the device driver should be deallocated. */
  void ( *device_deallocate )( libusb_device_t* device );
  /** Handler for checking for changes to the USB device tree. Only hubs need handle with this. */
  void ( *device_check_for_change )( libusb_device_t* device );
  /** Handler for removing a child device from this device. Only hubs need handle with this. */
  void ( *device_child_detached )( libusb_device_t* device, libusb_device_t* child );
  /** Handler for resetting a child device of this device. Only hubs need handle with this. */
  int ( *device_child_reset )( libusb_device_t* device, libusb_device_t* child );
  /** Handler for resetting a child device of this device. Only hubs need handle with this. */
  int ( *device_check_connection )( libusb_device_t* device, libusb_device_t* child );

  libusb_device_descriptor_t descriptor __aligned( 4 );
  libusb_configuration_descriptor_t configuration __aligned( 4 );
  libusb_interface_descriptor_t interfaces[ MAX_INTERFACES_PER_DEVICE ] __aligned( 4 );
  libusb_endpoint_descriptor_t endpoints[ MAX_INTERFACES_PER_DEVICE ][ MAX_ENDPOINTS_PER_DEVICE ] __aligned( 4 );
  libusb_device_t* parent __aligned( 4 );
  void *full_configuration;
  libusb_driver_data_header* driver_data;
  uint32_t last_transfer;

  // pointer to next usb device
  libusb_device_t* next;
  libusb_device_t* prev;
} libusb_device_t;

typedef enum {
  LIBUSB_HUB_PORT_CONTROL_GLOBAL = 0,
  LIBUSB_HUB_PORT_CONTROL_INDIVIDUAL = 1,
  LIBUSB_HUB_PORT_CONTROL_NO_POWER_SWITCHING = 2,
} libusb_hub_port_control_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type;
  uint8_t port_count;
  struct __packed {
    libusb_hub_port_control_t power_switching_mode : 2;
    bool compound : 1;
    libusb_hub_port_control_t over_current_protection : 2;
    uint8_t think_time : 2;
    bool indicators : 1;
    uint8_t reserved : 8;
  } attributes;
  uint8_t power_good_delay;
  uint8_t maximum_hub_power;
  uint8_t data[];
} libusb_hub_descriptor_t;

typedef enum {
  LIBUSB_HID_COUNTRY_NOT_SUPPORTED = 0,
  LIBUSB_HID_COUNTRY_ARABIC = 1,
  LIBUSB_HID_COUNTRY_BELGIAN = 2,
  LIBUSB_HID_COUNTRY_CANADIAN_BILINGUAL = 3,
  LIBUSB_HID_COUNTRY_CANADIAN_FRENCH = 4,
  LIBUSB_HID_COUNTRY_CZECH_REPUBLIC = 5,
  LIBUSB_HID_COUNTRY_DANISH = 6,
  LIBUSB_HID_COUNTRY_FINNISH = 7,
  LIBUSB_HID_COUNTRY_FRENCH = 8,
  LIBUSB_HID_COUNTRY_GERMAN = 9,
  LIBUSB_HID_COUNTRY_GREEK = 10,
  LIBUSB_HID_COUNTRY_HEBREW = 11,
  LIBUSB_HID_COUNTRY_HUNGARY = 12,
  LIBUSB_HID_COUNTRY_INTERNATIONAL = 13,
  LIBUSB_HID_COUNTRY_ITALIAN = 14,
  LIBUSB_HID_COUNTRY_JAPAN = 15,
  LIBUSB_HID_COUNTRY_KOREAN = 16,
  LIBUSB_HID_COUNTRY_LATIN_AMERICAN = 17,
  LIBUSB_HID_COUNTRY_DUTCH = 18,
  LIBUSB_HID_COUNTRY_NORWEGIAN = 19,
  LIBUSB_HID_COUNTRY_PERSIAN = 20,
  LIBUSB_HID_COUNTRY_POLAND = 21,
  LIBUSB_HID_COUNTRY_PORTUGUESE = 22,
  LIBUSB_HID_COUNTRY_RUSSIAN = 23,
  LIBUSB_HID_COUNTRY_SLOVAKIAN = 24,
  LIBUSB_HID_COUNTRY_SPANISH = 25,
  LIBUSB_HID_COUNTRY_SWEDISH = 26,
  LIBUSB_HID_COUNTRY_SWISS_FRENCH = 27,
  LIBUSB_HID_COUNTRY_SWISS_GERMAN = 28,
  LIBUSB_HID_COUNTRY_SWITZERLAND = 29,
  LIBUSB_HID_COUNTRY_TAIWAN = 30,
  LIBUSB_HID_COUNTRY_TURKISH_Q = 31,
  LIBUSB_HID_COUNTRY_ENGLISH_UK = 32,
  LIBUSB_HID_COUNTRY_ENGLISH_US = 33,
  LIBUSB_HID_COUNTRY_YUGOSLAVIAN = 34,
  LIBUSB_HID_COUNTRY_TURKISH_F = 35,
} libusb_hid_country_t;

typedef struct __packed {
  libusb_descriptor_type_t descriptor_type;
  uint16_t length;
} libusb_hid_optional_descriptor_t;

typedef struct __packed {
  uint8_t descriptor_length;
  libusb_descriptor_type_t descriptor_type : 8;
  uint16_t hid_version;
  libusb_hid_country_t hid_country : 8;
  uint8_t descriptor_count;
  libusb_hid_optional_descriptor_t optional[];
} libusb_hid_descriptor_t;

typedef struct __packed {
  bool local_power : 1;
  bool over_current : 1;
  uint16_t reserved : 14;
} libusb_hub_status_t;

typedef struct __packed {
  bool local_power_changed : 1;
  bool over_current_changed : 1;
  uint16_t reserved : 14;
} libusb_hub_status_change_t;

typedef struct __packed {
  libusb_hub_status_t status;
  libusb_hub_status_change_t change;
} libusb_hub_full_status_t;

typedef struct __packed {
  bool connected : 1;
  bool enabled : 1;
  bool suspended : 1;
  bool over_current : 1;
  bool reset : 1;
  uint8_t reserved0 : 3;
  bool power : 1;
  bool low_speed_attached : 1;
  bool high_speed_attached : 1;
  bool test_mode : 1;
  bool indicator_control : 1;
  uint8_t reserved1 : 3;
} libusb_hub_port_status_t;

typedef struct __packed {
  bool connected_changed : 1;
  bool enabled_changed : 1;
  bool suspended_changed : 1;
  bool over_current_changed : 1;
  bool reset_changed : 1;
  uint16_t reserved : 11;
} libusb_hub_port_status_change_t;

typedef struct __packed {
  libusb_hub_port_status_t status;
  libusb_hub_port_status_change_t change;
} libusb_hub_port_full_status_t;

typedef enum {
  LIBUSB_HUB_PORT_FEATURE_CONNECTION = 0,
  LIBUSB_HUB_PORT_FEATURE_ENABLE = 1,
  LIBUSB_HUB_PORT_FEATURE_SUSPEND = 2,
  LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT = 3,
  LIBUSB_HUB_PORT_FEATURE_RESET = 4,
  LIBUSB_HUB_PORT_FEATURE_POWER = 8,
  LIBUSB_HUB_PORT_FEATURE_LOW_SPEED = 9,
  LIBUSB_HUB_PORT_FEATURE_HIGH_SPEED = 10,
  LIBUSB_HUB_PORT_FEATURE_CONNECTION_CHANGE = 16,
  LIBUSB_HUB_PORT_FEATURE_ENABLE_CHANGE = 17,
  LIBUSB_HUB_PORT_FEATURE_SUSPENDED_CHANGE = 18,
  LIBUSB_HUB_PORT_FEATURE_OVER_CURRENT_CHANGE = 19,
  LIBUSB_HUB_PORT_FEATURE_RESET_CHANGE = 20,
} libusb_hub_port_feature_t;

typedef enum {
  DEVICE_DRIVER_HUB = 0x48554230,
  DEVICE_DRIVER_HID = 0x48494430,
  DEVICE_DRIVER_KEYBOARD = 0x4b424430,
  DEVICE_DRIVER_MOUSE = 0x4b424431,
} device_driver_t;

typedef struct libusb_hub_device libusb_hub_device_t;
typedef struct libusb_hub_device {
  libusb_driver_data_header header;
  libusb_hub_full_status_t status;
  libusb_hub_descriptor_t* descriptor;
  uint32_t max_children;
  libusb_hub_port_full_status_t port_status[ 255 ];
  uint32_t children[ 255 ];

  uint32_t device_number;

  libusb_hub_device_t* next;
  libusb_hub_device_t* prev;
} libusb_hub_device_t;

typedef enum {
  LIBUSB_HUB_FEATURE_POWER = 0,
  LIBUSB_HUB_FEATURE_OVER_CURRENT = 1,
} libusb_hub_feature_t;

typedef enum {
  LIBUSB_HID_REPORT_TAG_MAIN_INPUT = 0x20,
  LIBUSB_HID_REPORT_TAG_MAIN_OUTPUT = 0x24,
  LIBUSB_HID_REPORT_TAG_MAIN_FEATURE = 0x2c,
  LIBUSB_HID_REPORT_TAG_MAIN_COLLECTION = 0x28,
  LIBUSB_HID_REPORT_TAG_MAIN_END_COLLECTION = 0x30,
  LIBUSB_HID_REPORT_TAG_GLOBAL_USAGE_PAGE = 0x1,
  LIBUSB_HID_REPORT_TAG_GLOBAL_LOGICAL_MINIMUM = 0x5,
  LIBUSB_HID_REPORT_TAG_GLOBAL_LOGICAL_MAXIMUM = 0x9,
  LIBUSB_HID_REPORT_TAG_GLOBAL_PHYSICAL_MINIMUM = 0xd,
  LIBUSB_HID_REPORT_TAG_GLOBAL_PHYSICAL_MAXIMUM = 0x11,
  LIBUSB_HID_REPORT_TAG_GLOBAL_UNIT_EXPONENT = 0x15,
  LIBUSB_HID_REPORT_TAG_GLOBAL_UNIT = 0x19,
  LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_SIZE = 0x1d,
  LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_ID = 0x21,
  LIBUSB_HID_REPORT_TAG_GLOBAL_REPORT_COUNT = 0x25,
  LIBUSB_HID_REPORT_TAG_GLOBAL_PUSH = 0x29,
  LIBUSB_HID_REPORT_TAG_GLOBAL_POP = 0x2d,
  LIBUSB_HID_REPORT_TAG_LOCAL_USAGE = 0x2,
  LIBUSB_HID_REPORT_TAG_LOCAL_USAGE_MINIMUM = 0x6,
  LIBUSB_HID_REPORT_TAG_LOCAL_USAGE_MAXIMUM = 0xa,
  LIBUSB_HID_REPORT_TAG_LOCAL_DESIGNATOR_INDEX = 0xe,
  LIBUSB_HID_REPORT_TAG_LOCAL_DESIGNATOR_MINIMUM = 0x12,
  LIBUSB_HID_REPORT_TAG_LOCAL_DESIGNATOR_MAXIMUM = 0x16,
  LIBUSB_HID_REPORT_TAG_LOCAL_STRING_INDEX = 0x1e,
  LIBUSB_HID_REPORT_TAG_LOCAL_STRING_MINIMUM = 0x22,
  LIBUSB_HID_REPORT_TAG_LOCAL_STRING_MAXIMUM = 0x26,
  LIBUSB_HID_REPORT_TAG_LOCAL_DELIMITER = 0x2a,
  LIBUSB_HID_REPORT_TAG_LONG = 0x3f,
} libusb_hid_report_tag_t;

typedef struct __packed {
  uint8_t size : 2;
  libusb_hid_report_tag_t tag : 6;
} libusb_hid_report_item_t;

typedef struct __packed {
  bool constant : 1;
  bool variable : 1;
  bool relative : 1;
  bool wrap : 1;
  bool non_linear : 1;
  bool no_preferred : 1;
  bool hull : 1;
  bool _volatile : 1;
  bool buffered_bytes : 1;
  uint32_t reserved : 23;
} libusb_hid_main_item_t;

typedef enum {
  LIBUSB_HID_MAIN_COLLECTION_PHYSICAL = 0,
  LIBUSB_HID_MAIN_COLLECTION_APPLICATION = 1,
  LIBUSB_HID_MAIN_COLLECTION_LOGICAL = 2,
  LIBUSB_HID_MAIN_COLLECTION_REPORT = 3,
  LIBUSB_HID_MAIN_COLLECTION_NAMED_ARRAY = 4,
  LIBUSB_HID_MAIN_COLLECTION_USAGE_SWITCH = 5,
  LIBUSB_HID_MAIN_COLLECTION_USAGE_MODIFIER = 6,
} libusb_hid_main_collection_t;

typedef enum {
  LIBUSB_HID_USAGE_PAGE_UNDEFINED = 0,
  LIBUSB_HID_USAGE_PAGE_GENERIC_DESKTOP_CONTROL = 1,
  LIBUSB_HID_USAGE_PAGE_SIMULATION_CONTROL = 2,
  LIBUSB_HID_USAGE_PAGE_VR_CONTROL = 3,
  LIBUSB_HID_USAGE_PAGE_SPORT_CONTROL = 4,
  LIBUSB_HID_USAGE_PAGE_GAME_CONTROL = 5,
  LIBUSB_HID_USAGE_PAGE_GENERIC_DEVICE_CONTROL = 6,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_CONTROL = 7,
  LIBUSB_HID_USAGE_PAGE_LED = 8,
  LIBUSB_HID_USAGE_PAGE_BUTTON = 9,
  LIBUSB_HID_USAGE_PAGE_ORDINAL = 10,
  LIBUSB_HID_USAGE_PAGE_TELEPHONY = 11,
  LIBUSB_HID_USAGE_PAGE_CONSUMER = 12,
  LIBUSB_HID_USAGE_PAGE_DIGITIZER = 13,
  LIBUSB_HID_USAGE_PAGE_PID_PAGE = 15,
  LIBUSB_HID_USAGE_PAGE_UNICODE = 16,
  LIBUSB_HID_USAGE_PAGE_USAGE_PAGE = 0xffff,
} libusb_hid_usage_page_t;

typedef enum {
  LIBUSB_HID_USAGE_PAGE_DESKTOP_POINT = 1,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_MOUSE = 2,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_JOYSTICK = 4,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_GAMEPAD = 5,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_KEYBOARD = 6,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_KEYPAD = 7,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_MULTI_AXIS_CONTROLLER = 8,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_TABLE_PC_CONTROL = 9,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_X = 0x30,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_Y = 0x31,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_Z = 0x32,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_RX = 0x33,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_RY = 0x34,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_RZ = 0x35,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_SLIDER = 0x36,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_DIAL = 0x37,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_WHEEL = 0x38,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_HAT_SWITCH = 0x39,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_COUNTED_BUFFER = 0x3a,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_BYTE_COUNT = 0x3b,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_MOTION_WAKE_UP = 0x3c,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_START = 0x3d,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_SELECT = 0x3e,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VX = 0x40,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VY = 0x41,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VZ = 0x42,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VBR_X = 0x43,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VBR_Y = 0x44,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VBR_Z = 0x45,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_VNO = 0x46,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_FEATURE_NOTIFICATION = 0x47,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_RESOLUTION_MULTIPLIER = 0x48,
  LIBUSB_HID_USAGE_PAGE_DESKTOP_DUMMY = 0xffff,
} libusb_hid_usage_page_desktop_t;

typedef enum {
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_ERROR_ROLL_OVER = 1,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_POST_FAIL = 2,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_ERROR_UNDEFINED = 3,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_CONTROL = 0xe0,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_SHIFT = 0xe1,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_ALT = 0xe2,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_LEFT_GUI = 0xe3,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_RIGHT_CONTROL = 0xe4,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_RIGHT_SHIFT = 0xe5,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_RIGHT_ALT = 0xe6,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_RIGHT_GUI = 0xe7,
  LIBUSB_HID_USAGE_PAGE_KEYBOARD_DUMMY = 0xffff,
} libusb_hid_usage_page_keyboard_t;

typedef enum {
  LIBUSB_HID_USAGE_PAGE_LED_NUMBER_LOCK = 1,
  LIBUSB_HID_USAGE_PAGE_LED_CAPSLOCK = 2,
  LIBUSB_HID_USAGE_PAGE_LED_SCROLL_LOCK = 3,
  LIBUSB_HID_USAGE_PAGE_LED_COMPOSE = 4,
  LIBUSB_HID_USAGE_PAGE_LED_KANA = 5,
  LIBUSB_HID_USAGE_PAGE_LED_POWER = 6,
  LIBUSB_HID_USAGE_PAGE_LED_SHIFT = 7,
  LIBUSB_HID_USAGE_PAGE_LED_MUTE = 9,
  LIBUSB_HID_USAGE_PAGE_LED_DUMMY = 0xffff,
} libusb_hid_usage_page_led_t;

typedef enum {
  LIBUSB_HID_REPORT_TYPE_INPUT = 1,
  LIBUSB_HID_REPORT_TYPE_OUTPUT = 2,
  LIBUSB_HID_REPORT_TYPE_FEATURE = 3,
} libusb_hid_report_type_t;

typedef struct __packed {
  union {
    libusb_hid_usage_page_desktop_t desktop : 16;
    libusb_hid_usage_page_keyboard_t keyboard : 16;
    libusb_hid_usage_page_led_t led : 16;
  };
  libusb_hid_usage_page_t page : 16;
} libusb_hid_full_usage_t;

typedef enum {
  LIBUSB_HID_UNIT_SYSTEM_NONE = 0,
  LIBUSB_HID_UNIT_SYSTEM_STANDARD_LINEAR = 1,
  LIBUSB_HID_UNIT_SYSTEM_STANDARD_ROTATION = 2,
  LIBUSB_HID_UNIT_SYSTEM_ENGLISH_LINEAR = 3,
  LIBUSB_HID_UNIT_SYSTEM_ENGLISH_ROTATION = 4,
} libusb_hid_unit_system_t;

typedef struct __packed {
  libusb_hid_unit_system_t system : 4;
  int8_t length : 4;
  int8_t mass : 4;
  int8_t time : 4;
  int8_t temperature : 4;
  int8_t current : 4;
  int8_t luminous_intensity : 4;
  uint8_t reserved : 4;
} libusb_hid_unit_t;

typedef struct {
  uint8_t size;
  uint8_t offset;
  uint8_t count;
  libusb_hid_main_item_t attribute __aligned(4);
  libusb_hid_full_usage_t usage;
  libusb_hid_full_usage_t physical_usage;
  int32_t logical_minimum;
  int32_t logical_maximum;
  int32_t physical_minimum;
  int32_t physical_maximum;
  libusb_hid_unit_t unit;
  int32_t unit_exponent;
  union {
    uint8_t u8;
    int8_t i8;
    uint16_t u16;
    int16_t i16;
    uint32_t u32;
    int32_t i32;
    bool _bool;
    void* ptr;
  } value;
} libusb_hid_parser_fields_t;

typedef struct {
  uint8_t index;
  uint8_t field_count;
  uint8_t id;
  libusb_hid_report_type_t type;
  uint8_t report_length;
  uint8_t* report_buffer;
  size_t fields_length;
  libusb_hid_parser_fields_t fields[] __aligned(4);
} libusb_hid_parser_report_t;

typedef struct {
  libusb_hid_full_usage_t application;
  uint8_t report_count;
  uint8_t interface;
  libusb_hid_parser_report_t* report[] __aligned(4);
} libusb_hid_parser_result_t;

typedef struct libusb_hid_device libusb_hid_device_t;
typedef struct libusb_hid_device {
  libusb_driver_data_header header;
  libusb_hid_descriptor_t* descriptor;
  libusb_hid_parser_result_t* parser_result;
  libusb_driver_data_header* driver_data;

  pid_t device_detached_handler;
  pid_t device_deallocate_handler;

  uint32_t device_number;

  libusb_hid_device_t* next;
  libusb_hid_device_t* prev;
} libusb_hid_device_t;

typedef struct __packed {
  bool left_control : 1;
  bool left_shift : 1;
  bool left_alt : 1;
  bool left_gui : 1;
  bool right_control : 1;
  bool right_shift : 1;
  bool right_alt : 1;
  bool right_gui : 1;
} libusb_keyboard_modifier_t;

typedef struct __packed {
  bool num_lock : 1;
  bool caps_lock : 1;
  bool scroll_lock : 1;
  bool compose : 1;
  bool kana : 1;
  bool power : 1;
  bool mute : 1;
  bool shift : 1;
} libusb_keyboard_led_t;

#define KEYBOARD_REPORT_SIZE 8
#define KEYBOARD_MAX_KEYS 6

typedef struct libusb_keyboard_device libusb_keyboard_device_t;
typedef struct libusb_keyboard_device {
  libusb_driver_data_header header;
  uint32_t index;
  uint32_t key_count;
  uint16_t max_key_down[ KEYBOARD_MAX_KEYS ];
  libusb_keyboard_modifier_t modifier;
  libusb_keyboard_led_t led;
  libusb_hid_parser_fields_t* led_field[ 8 ];
  libusb_hid_parser_fields_t* key_field[ 8 + 1 ];
  libusb_hid_parser_report_t* led_report;
  libusb_hid_parser_report_t* key_report;

  uint32_t device_number;
  libusb_endpoint_descriptor_t descriptor;
  size_t last_poll;
  size_t running_poll;
  uint8_t* buffer;

  libusb_keyboard_device_t* next;
  libusb_keyboard_device_t* prev;
} libusb_keyboard_device_t;

typedef enum {
  LIBUSB_MOUSE_DEVICE_BUTTON_LEFT,
  LIBUSB_MOUSE_DEVICE_BUTTON_RIGHT,
  LIBUSB_MOUSE_DEVICE_BUTTON_MIDDLE,
  LIBUSB_MOUSE_DEVICE_BUTTON_SIDE,
  LIBUSB_MOUSE_DEVICE_BUTTON_EXTRA,
} libusb_mouse_device_button_t;

typedef struct libusb_mouse_device libusb_mouse_device_t;
typedef struct libusb_mouse_device {
  libusb_driver_data_header header;
  uint32_t index;

  uint8_t button_state;
  int16_t mouse_x;
  int16_t mouse_y;
  int16_t wheel;

  libusb_hid_parser_report_t* mouse_report;

  uint32_t device_number;
  libusb_endpoint_descriptor_t descriptor;
  size_t last_poll;
  uint8_t* buffer;

  libusb_mouse_device_t* next;
  libusb_mouse_device_t* prev;
} libusb_mouse_device_t;

// enable warnings again
#pragma GCC diagnostic pop

#endif
