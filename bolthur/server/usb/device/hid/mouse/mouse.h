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

#ifndef _MOUSE_H
#define _MOUSE_H

#include "../../../../libusb.h"

#define MOUSE_ENABLE_DEBUG 1

void mouse_append( libusb_mouse_device_t* );
int mouse_new_index( uint32_t* );
void mouse_destroy( libusb_mouse_device_t* );
int mouse_duplicate_report( libusb_hid_parser_report_t**, const libusb_hid_parser_report_t*, size_t );
int mouse_start_polling( libusb_mouse_device_t* );
libusb_mouse_device_t* mouse_get_device( uint32_t );

#endif
