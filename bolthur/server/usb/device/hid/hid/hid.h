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

#ifndef _HID_H
#define _HID_H

#include "../../../../libusb.h"

//#define HID_ENABLE_DEBUG 1
//#define HID_ENABLE_ERROR 1

// Protocol IDs
typedef enum {
  HID_PROTOCOL_BOOT = 0,
  HID_PROTOCOL_REPORT = 1,
} hid_protocol_t;

typedef struct {
  uint8_t count;
  uint8_t indent;
  bool input;
  bool output;
  bool feature;
} hid_report_action_count_t;

typedef struct {
  uint8_t report_id;
  uint8_t field_count;
  libusb_hid_report_type_t report_type;
} hid_report_field_data_t;

typedef struct {
  uint32_t count;
  uint8_t current;
  uint8_t report;
  hid_report_field_data_t data[];
} hid_report_field_t;

typedef struct {
  libusb_hid_parser_result_t* result;
  uint32_t count;
  uint32_t size;
  libusb_hid_full_usage_t* usage;
  libusb_hid_full_usage_t physical;
  int32_t logical_minimum;
  int32_t logical_maximum;
  int32_t physical_minimum;
  int32_t physical_maximum;
  libusb_hid_unit_t unit;
  int32_t unit_exponent;
  libusb_hid_usage_page_t page;
  uint8_t report;
} hid_field_t;

typedef void( *hid_report_action_t )( void** data, libusb_hid_report_tag_t tag, uint32_t value );

void hid_destroy_device( libusb_hid_device_t* );
int hid_set_protocol( uint32_t, uint16_t, hid_protocol_t );
int hid_set_idle( uint32_t, uint16_t, uint8_t, uint8_t );
void hid_enumerate_action_count_report( void**, libusb_hid_report_tag_t, uint32_t );
void hid_enumerate_action_count_field_process( hid_report_field_t**, uint32_t, libusb_hid_report_type_t );
void hid_enumerate_action_count_field( void**, libusb_hid_report_tag_t, uint32_t );
void hid_enumerate_action_add_field_process( hid_field_t**, uint32_t, libusb_hid_report_type_t );
void hid_enumerate_action_add_field( void** data, libusb_hid_report_tag_t, uint32_t );
void hid_enumerate_report( void*, size_t, hid_report_action_t, void** );
int hid_parse_report_descriptor( libusb_hid_device_t*, void*, size_t );
void hid_append( libusb_hid_device_t* );
int hid_get( uint32_t, libusb_hid_device_t** );
int hid_set( uint32_t, void*, size_t );

#endif
