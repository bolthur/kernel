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

typedef enum {
  LIBUSB_SPEED_HIGH = 0,
  LIBUSB_SPEED_FULL = 1,
  LIBUSB_SPEED_LOW = 2,
} libusb_speed_t;

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

// enable warnings again
#pragma GCC diagnostic pop

#endif
