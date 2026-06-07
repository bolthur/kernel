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

#ifndef _LIBUSBD_LOCAL_H
#define _LIBUSBD_LOCAL_H

#include "../../libusbd.h"

//#define USBD_ENABLE_DEBUG 1
//#define USBD_ENABLE_ERROR 1

// if one of both is defined include inttypes for printing stuff
#if defined( USBD_ENABLE_DEBUG ) || defined( USBD_ENABLE_ERROR )
  #include <inttypes.h>
#endif

/**
 * @brief Control message timeout in milliseconds
 */
#define CONTROL_MESSAGE_TIMEOUT 10

/**
 * @brief usbd attach context for async attach
 */
typedef struct {
  /** original request */
  void* request;
  /** request size */
  size_t request_size;
  /** origin */
  pid_t origin;
  /** data info */
  size_t data_info;
  /** handler to be called on finish */
  rpc_handler_t handler;
} usbd_attach_context_t;

// address
int usbd_address_set( libusb_device_t*, uint8_t );
// allocate
int usbd_allocate_device( libusb_device_t**, bool );
// attach
int usbd_attach_device( libusb_device_t*, rpc_handler_t, pid_t, size_t, const void*, size_t );
// configuration
int usbd_configuration_set( libusb_device_t*, uint8_t );
// control
int usbd_control_message( libusb_device_t*, libusb_pipe_address_t, void*, size_t, const libusb_device_request_t*, size_t );
int usbd_control_message_async( const libusb_device_t*, libusb_pipe_address_t, const void*, size_t, const libusb_device_request_t*, size_t, rpc_handler_t, pid_t, size_t, void*, size_t );
// deallocate
void usbd_deallocate_device( libusb_device_t* );
// description
const char* usbd_description_get( const libusb_device_t* );
// descriptor
int usbd_descriptor_get( libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t );
int usbd_descriptor_get_async( const libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, const void*, size_t, uint8_t, rpc_handler_t, pid_t, size_t, void*, size_t );
int usbd_descriptor_read_device( libusb_device_t* );
// device
int usbd_device_configure( libusb_device_t*, uint8_t );
int usbd_device_get_by_number( uint32_t, libusb_device_t** );
// handler
int usbd_handler_init( void );
int usbd_handler_register( libusb_interface_class_t, pid_t );
int usbd_handler_unregister( libusb_interface_class_t, pid_t );
int usbd_handler_get( libusb_interface_class_t, pid_t* );
// init
int usbd_init( void );
// interrupt
int usbd_interrupt_poll( const libusb_device_t*, libusb_pipe_address_t, const void*, size_t, size_t, uint8_t, uint32_t, rpc_handler_t, pid_t, size_t, void*, size_t );
// roothub
libusb_device_t* usbd_roothub_get( void );
int usbd_roothub_attach( void );
// string
int usbd_string_get( libusb_device_t*, uint8_t, uint16_t, void*, size_t );
int usbd_string_read_lang( libusb_device_t*, uint8_t, uint16_t, void*, size_t );
int usbd_string_read( libusb_device_t*, uint8_t, void*, size_t );

// global variables from init
extern int fd_hcd;
extern libusb_device_t* head;

#endif
