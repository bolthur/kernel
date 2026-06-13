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

#ifndef CALL_H
#define CALL_H

#include "libusbd.h"

//#define CALL_ENABLE_DEBUG 1
// if call debug is defined include inttypes for printing stuff
#if defined( CALL_ENABLE_DEBUG )
  #include <inttypes.h>
#endif

int call_attach( libusb_device_t*, uint32_t, rpc_handler_t, usbd_attach_context_t* );
int call_detached( const libusb_device_t* );
int call_deallocate( const libusb_device_t* );
int call_check_for_change( const libusb_device_t* );
int call_child_detached( const libusb_device_t*, const libusb_device_t* );
int call_child_reset( const libusb_device_t*, const libusb_device_t* );
int call_child_check_connection( const libusb_device_t*, const libusb_device_t* );

#endif
