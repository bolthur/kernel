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

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../../../../libusb.h"

#define KEYBOARD_ENABLE_DEBUG 1
#define KEYBOARD_ENABLE_ERROR 1

extern libusb_keyboard_device_t* keyboard_head;

void keyboard_append( libusb_keyboard_device_t* );
void keyboard_destroy( libusb_keyboard_device_t* );
int keyboard_new_index( uint32_t* );
int keyboard_duplicate_report( libusb_hid_parser_report_t**, const libusb_hid_parser_report_t*, size_t );
int keyboard_start_polling( libusb_keyboard_device_t* );
libusb_keyboard_device_t* keyboard_get_device(uint32_t);
void keyboard_bit_set( uint8_t*, uint32_t, uint32_t, uint32_t );
int32_t keyboard_bit_get_signed( const uint8_t*, uint32_t, uint32_t );
uint32_t keyboard_bit_get_unsigned( const uint8_t*, uint32_t, uint32_t );
int32_t keyboard_bit_get_value( const libusb_hid_parser_fields_t*, uint32_t );

#endif
