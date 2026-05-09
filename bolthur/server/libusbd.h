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

#ifndef _LIBUSBD_H
#define _LIBUSBD_H

#include <sys/bolthur.h>
#include "libusb.h"

// device paths
#define USBD_DEVICE_PATH "/dev/usb/usbd"
#define HUB_DEVICE_PATH "/dev/usb/hub"
#define HID_DEVICE_PATH "/dev/usb/hid"
#define KEYBOARD_DEVICE_PATH "/dev/usb/keyboard"
#define MOUSE_DEVICE_PATH "/dev/usb/mouse"

// generic rpc
#define GENERIC_ATTACH RPC_CUSTOM_START
#define GENERIC_DETACH GENERIC_ATTACH + 1
#define GENERIC_DEALLOCATE GENERIC_DETACH + 1

// hid rpc
#define HID_REGISTER_HANDLER GENERIC_DEALLOCATE + 1
#define HID_UNREGISTER_HANDLER HID_REGISTER_HANDLER + 1
#define HID_GET_DRIVER HID_UNREGISTER_HANDLER + 1
#define HID_GET_APPLICATION HID_GET_DRIVER + 1
#define HID_GET_REPORT_COUNT HID_GET_APPLICATION + 1
#define HID_GET_REPORT HID_GET_REPORT_COUNT + 1

// hub rpc
#define HUB_CHECK_CHANGE GENERIC_DEALLOCATE + 1
#define HUB_CHILD_DETACH HUB_CHECK_CHANGE + 1
#define HUB_CHILD_RESET HUB_CHILD_DETACH + 1
#define HUB_CHECK_CONNECTION HUB_CHILD_RESET + 1

// usbd rpc
#define USBD_REGISTER_HANDLER RPC_CUSTOM_START
#define USBD_UNREGISTER_HANDLER USBD_REGISTER_HANDLER + 1
#define USBD_GET_DESCRIPTOR USBD_UNREGISTER_HANDLER + 1
#define USBD_GET_ENDPOINT USBD_GET_DESCRIPTOR + 1
#define USBD_GET_INTERFACE USBD_GET_ENDPOINT + 1
#define USBD_GET_DESCRIPTION USBD_GET_INTERFACE + 1
#define USBD_CONTROL_MESSAGE USBD_GET_DESCRIPTION + 1
#define USBD_ATTACH_DEVICE USBD_CONTROL_MESSAGE + 1
#define USBD_GET_ROOTHUB USBD_ATTACH_DEVICE + 1
#define USBD_GET_CONFIGURATION USBD_GET_ROOTHUB + 1
#define USBD_GET_STATUS USBD_GET_CONFIGURATION + 1

// generic usb rpc structures
typedef struct {
  uint32_t parent_device_number;
  uint32_t device_number;
  uint32_t interface_number;
} usb_generic_attach_t;

// hid rpc structures
typedef struct {
  libusb_hid_usage_page_desktop_t type;
  pid_t handler;
} hid_register_device_handler_t;

typedef struct {
  libusb_hid_usage_page_desktop_t type;
  pid_t handler;
} hid_unregister_device_handler_t;

typedef struct {
  uint32_t device_number;
  uint32_t device_driver;
} hid_get_driver_t;

typedef struct {
  uint32_t device_number;
  libusb_hid_full_usage_t application;
} hid_get_application_t;

typedef struct {
  uint32_t device_number;
  uint8_t report_count;
} hid_get_report_count_t;

typedef struct {
  uint32_t device_number;
  uint8_t report;
  size_t shm_id;
} hid_get_report_t;

// usbd rpc structures
typedef struct {
  libusb_interface_class_t type;
  pid_t handler;
} usbd_register_device_handler_t;

typedef struct {
  libusb_interface_class_t type;
  pid_t handler;
} usbd_unregister_device_handler_t;

typedef struct {
  size_t shm_id;
} usbd_get_descriptor_t;

typedef struct {
  uint32_t device_number;
  libusb_descriptor_type_t type;
  uint8_t index;
  uint16_t lang_id;
  size_t buffer_length;
  size_t minimum_length;
  uint8_t recipient;
  uint8_t buffer[];
} usb_descriptor_message_t;

typedef struct {
  // parameters
  uint32_t device_number;
  uint32_t interface_number;
  uint32_t endpoint_number;
  // space for return object
  libusb_endpoint_descriptor_t descriptor;
} usbd_get_endpoint_t;

typedef struct {
  // parameters
  uint32_t device_number;
  uint32_t interface_number;
  // space for return
  libusb_interface_descriptor_t interface;
} usbd_get_interface_t;

typedef struct {
  uint32_t device_number;
  char buffer[ 256 ];
} usbd_get_description_t;

typedef struct {
  size_t shm_id;
} usbd_control_message_t;

typedef struct {
  uint32_t device_number;
} usbd_get_roothub_t;

typedef struct {
  uint32_t parent_number;
  uint32_t port_number;
  libusb_speed_t speed;
} usbd_attach_device_t;

typedef struct {
  uint32_t device_number;
  libusb_transfer_t transfer;
  libusb_direction_t direction;
  size_t buffer_length;
  libusb_device_request_t request;
  size_t timeout;
  uint32_t last_transfer;
  libusb_transfer_error_t error;
  uint8_t buffer[];
} usb_control_message_t;

typedef struct {
  uint32_t device_number;
  uint32_t configuration_length;
  size_t shm_id;
} usbd_get_configuration_t;

typedef struct {
  uint32_t device_number;
  libusb_device_status_t status;
} usbd_get_status_t;

#endif
