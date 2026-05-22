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

#ifndef _HUB_H
#define _HUB_H

#include "../../../libusb.h"

//#define HUB_ENABLE_DEBUG 1

void hub_append( libusb_hub_device_t* );
int hub_read_descriptor( uint32_t, void** );
int hub_get_status( uint32_t, libusb_hub_device_t* );
int hub_change_port_feature( uint32_t, libusb_hub_port_feature_t, uint8_t, bool );
int hub_power_on( uint32_t, const libusb_hub_device_t* );
int hub_get_port_status( uint32_t, libusb_hub_device_t*, uint8_t );
int hub_port_reset( uint32_t, libusb_hub_device_t*, uint8_t );
int hub_port_connection_changed( uint32_t, libusb_hub_device_t*, uint8_t );
int hub_check_connection( uint32_t, libusb_hub_device_t*, uint8_t );

#endif
