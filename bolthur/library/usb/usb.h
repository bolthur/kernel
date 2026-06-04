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

#ifndef _USB_H
#define _USB_H

#include "../../server/libusb.h"

//#define LIBUSB_ENABLE_DEBUG 1
//#define LIBUSB_ENABLE_ERROR 1

int usb_init( void );
int usb_control_message( uint32_t, libusb_transfer_t, libusb_direction_t, void*, size_t, const libusb_device_request_t*, size_t, libusb_transfer_error_t*, uint32_t* );
int usb_control_message_async( uint32_t, libusb_transfer_t, libusb_direction_t, const void*, size_t, const libusb_device_request_t*, size_t, rpc_handler_t );
int usb_interrupt_poll_async( uint32_t, libusb_transfer_t, uint32_t, libusb_direction_t, const void*, size_t, size_t, uint8_t, uint32_t, rpc_handler_t );
const char* usb_get_description( uint32_t );
int usb_get_descriptor( uint32_t, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t );
int usb_attach_device( uint32_t, uint32_t, libusb_speed_t );
int usb_register_handler( libusb_interface_class_t );
int usb_get_endpoint( uint32_t, uint32_t, uint32_t, libusb_endpoint_descriptor_t* );
int usb_get_interface( uint32_t, uint32_t, libusb_interface_descriptor_t* );
int usb_get_root_hub( uint32_t* );
int usb_get_configuration( uint32_t, void** );
int usb_get_status( uint32_t, libusb_device_status_t* );

#endif
