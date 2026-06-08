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

#ifndef _HANDLER_H
#define _HANDLER_H

#include "../../../../libusb.h"
#include "../../../../../library/collection/avl/avl.h"

typedef struct {
  avl_node_t node;
  pid_t handler;
} pid_container_t;

#define PID_HANDLER_GET_ENTRY( n ) \
  ( pid_container_t* )( ( uint8_t* )n - offsetof( pid_container_t, node ) )

int handler_init( void );
int handler_register( libusb_hid_usage_page_desktop_t, pid_t );
int handler_unregister( libusb_hid_usage_page_desktop_t, pid_t );
int handler_get( libusb_hid_usage_page_desktop_t, pid_t* );
int handler_call_attach( libusb_hid_usage_page_desktop_t, libusb_hid_device_t*, uint32_t, uint32_t, rpc_handler_t, pid_t, size_t );

#endif
