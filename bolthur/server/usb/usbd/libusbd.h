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
 * @brief Min descriptor read length
 */
#define DESCRIPTOR_READ_MIN_LENGTH 8

/**
 * @brief usbd attach context for async attach
 */
typedef struct {
  /** original request */
  void* request;
  /** request size */
  size_t request_size;
  /** origin modified in call chain */
  pid_t origin;
  /** data info modified in call chain */
  size_t data_info;
  /** handler to be called on finish */
  rpc_handler_t handler;
  /** device */
  uint8_t device_number;
  /** address to use */
  uint8_t address;
  /** device itself to attach */
  libusb_device_t* device;
} usbd_attach_context_t;

/**
 * @brief usbd descriptor context
 */
typedef struct {
  /** original context */
  usbd_attach_context_t* context;
  /** handler to be called once finished */
  rpc_handler_t handler;
} usbd_descriptor_context_t;

/**
 * @brief usbd address context
 */
typedef struct {
  /** original context */
  usbd_attach_context_t* context;
  /** handler to be called once finished */
  rpc_handler_t handler;
  /** address that is going to be set */
  uint8_t address;
} usbd_address_context_t;

/**
 * @brief usbd configure context
 */
typedef struct {
  /** original context */
  usbd_attach_context_t* context;
  /** handler to be called once finished */
  rpc_handler_t handler;
  /** configuration */
  uint8_t configuration;
  /** full configuration descriptor */
  void* full_descriptor;
} usbd_configure_context_t;

/**
 * @brief usbd configuration context
 */
typedef struct {
  /** original context */
  usbd_configure_context_t* context;
  /** handler to be called once finished */
  rpc_handler_t handler;
  /** configuration */
  uint8_t configuration;
} usbd_configuration_context_t;

/**
 * @brief usbd control context
 */
typedef struct {
  /** handler to be called once finished */
  rpc_handler_t handler;
  /** other context */
  void* context;
} usbd_control_context_t;

// address
int usbd_address_set( libusb_device_t*, uint8_t, rpc_handler_t, usbd_attach_context_t* );
// allocate
int usbd_allocate_device( libusb_device_t**, bool );
// attach
int usbd_attach_device( libusb_device_t*, rpc_handler_t, pid_t, size_t, const void*, size_t );
// configuration
int usbd_configuration_set( libusb_device_t*, uint8_t, rpc_handler_t, usbd_configure_context_t* );
// context
int usbd_context_attach_create( rpc_handler_t, pid_t, size_t, const void*, size_t, uint8_t, uint8_t, libusb_device_t*, usbd_attach_context_t**);
void usbd_context_attach_destroy( usbd_attach_context_t* );
int usbd_context_descriptor_create( rpc_handler_t, void*, usbd_descriptor_context_t** );
void usbd_context_descriptor_destroy( usbd_descriptor_context_t* );
int usbd_context_address_create( rpc_handler_t, void*, uint8_t, usbd_address_context_t** );
void usbd_context_address_destroy( usbd_address_context_t* );
int usbd_context_configure_create( rpc_handler_t callback, void*, uint8_t, usbd_configure_context_t** );
void usbd_context_configure_destroy( usbd_configure_context_t* );
int usbd_context_configuration_create( rpc_handler_t, void*, uint8_t, usbd_configuration_context_t** );
void usbd_context_configuration_destroy( usbd_configuration_context_t* );
int usbd_context_control_create( rpc_handler_t, void*, usbd_control_context_t**);
void usbd_context_control_destroy( usbd_control_context_t* );
// control
int usbd_control_message( libusb_device_t*, libusb_pipe_address_t, void*, size_t, const libusb_device_request_t*, size_t );
int usbd_control_message_async( const libusb_device_t*, libusb_pipe_address_t, const void*, size_t, const libusb_device_request_t*, size_t, rpc_handler_t, pid_t, size_t, void*, size_t, void*, size_t );
// deallocate
void usbd_deallocate_device( libusb_device_t* );
// description
const char* usbd_description_get( const libusb_device_t* );
// descriptor
int usbd_descriptor_get( libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, void*, size_t, size_t, uint8_t );
int usbd_descriptor_get_async( const libusb_device_t*, libusb_descriptor_type_t, uint8_t, uint16_t, const void*, size_t, uint8_t, rpc_handler_t, pid_t, size_t, void*, size_t, void*, size_t );
int usbd_descriptor_read_device( libusb_device_t*, rpc_handler_t, usbd_attach_context_t* );
// device
int usbd_device_configure( libusb_device_t*, uint8_t, rpc_handler_t, usbd_attach_context_t* );
int usbd_device_get_by_number( uint32_t, libusb_device_t** );
// handler
int usbd_handler_init( void );
int usbd_handler_register( libusb_interface_class_t, pid_t );
int usbd_handler_unregister( libusb_interface_class_t, pid_t );
int usbd_handler_get( libusb_interface_class_t, pid_t* );
// init
int usbd_init( void );
// interrupt
int usbd_interrupt_poll( const libusb_device_t*, libusb_pipe_address_t, usb_interrupt_poll_t*, usbd_interrupt_message_t*, rpc_handler_t, pid_t, size_t, void*, size_t );
// roothub
libusb_device_t* usbd_roothub_get( void );
int usbd_roothub_attach( rpc_handler_t, pid_t, size_t );
int usbd_roothub_fire_attach( void );
// string
int usbd_string_get( libusb_device_t*, uint8_t, uint16_t, void*, size_t );
int usbd_string_read_lang( libusb_device_t*, uint8_t, uint16_t, void*, size_t );
int usbd_string_read( libusb_device_t*, uint8_t, void*, size_t );

// global variables from init
extern int fd_hcd;
extern libusb_device_t* head;

#endif
