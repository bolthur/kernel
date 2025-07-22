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

#ifndef _LIBUSB_H
#define _LIBUSB_H

#define USBD_DEVICE_PATH "/dev/usb/usbd"

#define MAX_CHILDREN_PER_DEVICE 10
#define MAX_INTERFACES_PER_DEVICE 8
#define MAX_ENDPOINTS_PER_DEVICE 16

// disable a bunch of warnings necessary to build packed structures
// for usb communication
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"

#include <stdint.h>

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

  /** Handler for detaching the device. The device driver should not issue further requests to the device. */
  void ( *device_detached )( libusb_device_t* device ) __aligned( 4 );
  /** Handler for deallocation of the device. All memory in use by the device driver should be deallocated. */
  void ( *device_deallocate )( libusb_device_t* device );
  /** Handler for checking for changes to the USB device tree. Only hubs need handle with this. */
  void ( *device_check_for_change )( libusb_device_t* device );
  /** Handler for removing a child device from this device. Only hubs need handle with this. */
  void ( *device_child_detached )( libusb_device_t* device, libusb_device_t* child );
  /** Handler for reseting a child device of this device. Only hubs need handle with this. */
  int ( *device_child_reset )( libusb_device_t* device, libusb_device_t* child );
  /** Handler for reseting a child device of this device. Only hubs need handle with this. */
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

#define DEVICE_DRIVER_HUB 0x48554230

typedef struct {
  libusb_driver_data_header header;
  libusb_hub_full_status_t status;
  libusb_hub_descriptor_t* descriptor;
  uint32_t max_children;
  libusb_hub_port_full_status_t port_status[ MAX_CHILDREN_PER_DEVICE ];
  libusb_device_t* children[ MAX_CHILDREN_PER_DEVICE ];
} libusb_hub_device_t;

typedef enum {
  LIBUSB_HUB_FEATURE_POWER = 0,
  LIBUSB_HUB_FEATURE_OVER_CURRENT = 1,
} libusb_hub_feature_t;

// enable warnings again
#pragma GCC diagnostic pop

#endif
