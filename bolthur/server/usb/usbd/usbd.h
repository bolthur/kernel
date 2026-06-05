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

#ifndef USBD_H
#define USBD_H

#include "../../libusbd.h"

//#define USBD_ENABLE_DEBUG 1
//#define USBD_ENABLE_ERROR 1

extern int fd_hcd;
extern libusb_device_t* head;

int usbd_get_descriptor( libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t );
int usbd_get_string( libusb_device_t*, uint8_t, uint16_t, void*, size_t );
int usbd_read_string_lang( libusb_device_t*, uint8_t, uint16_t, void*, size_t );
int usbd_read_string( libusb_device_t*, uint8_t, void*, size_t );
int usbd_read_device_descriptor( libusb_device_t* );
int usbd_set_address( libusb_device_t*, uint8_t );
int usbd_set_configuration( libusb_device_t*, uint8_t );
int usbd_configure( libusb_device_t*, uint8_t );
int usbd_attach_device( libusb_device_t* );
int usbd_attach_root_hub( void );
libusb_device_t* usbd_get_root_hub( void );
int usbd_init( void );

#endif
