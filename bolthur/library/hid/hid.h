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

#ifndef _HID_H
#define _HID_H

#include "../../server/libusb.h"

int hid_init( void );
int hid_register_handler( libusb_hid_usage_page_desktop_t );
int hid_get_driver( uint32_t, uint32_t* );
int hid_get_application( uint32_t, libusb_hid_full_usage_t* );
int hid_get_report_count( uint32_t, uint8_t* );
void hid_destroy_report( libusb_hid_parser_report_t* );
int hid_get_report( uint32_t, uint8_t, libusb_hid_parser_report_t** );

#endif
