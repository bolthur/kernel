/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <stdint.h>

#ifndef _FAT_NODE_H
#define _FAT_NODE_H

// file attribute defines
#define FILE_ATTRIBUTE_READ_ONLY 0x01
#define FILE_ATTRIBUTE_HIDDEN 0x02
#define FILE_ATTRIBUTE_SYSTEM 0x04
#define FILE_ATTRIBUTE_VOLUME_ID 0x08
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_ARCHIVE 0x20
#define FILE_ATTRIBUTE_LONG_FILE_NAME \
  FILE_ATTRIBUTE_READ_ONLY | FILE_ATTRIBUTE_HIDDEN \
  | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_VOLUME_ID
// helper to extract specific information from time field
#define TIME_EXTRACT_HOUR(x) ((x & 0x7C00) >> 11)
#define TIME_EXTRACT_MINUTE(x) ((x & 0x3F0) >> 5)
#define TIME_EXTRACT_SECOND(x) (x & 0x1F)
// helper to extract specific information from date field
#define DATE_EXTRACT_HOUR(x) ((x & 0xFE00) >> 9)
#define DATE_EXTRACT_MINUTE(x) ((x & 0x1E0) >> 5)
#define DATE_EXTRACT_SECOND(x) (x & 0x1F)

#pragma pack(push, 1)

// fat directory
typedef struct {
  char name[ 8 ];
  char extenstion[ 3 ];
  uint8_t attributes;
  uint8_t reserved0;
  uint8_t creation_time_tenths;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_accessed_date;
  uint16_t high_bits;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
  uint16_t low_bits;
  uint32_t file_size;
} fat_node_folder_file_t;

static_assert(
  32 == sizeof( fat_node_folder_file_t ),
  "invalid fat_node_folder_file_t size!"
);

// long file name structure
typedef struct {
  uint8_t order;
  char first_five_two_byte[ 10 ];
  uint8_t attribute;
  uint8_t type;
  uint8_t checksum;
  char next_six_two_byte[ 12 ];
  uint16_t zero;
  char final_two_byte[ 4 ];
} fat_node_long_filename_t;

static_assert(
  32 == sizeof( fat_node_long_filename_t ),
  "invalid fat_node_long_filename_t size!"
);

#pragma pack(pop)

#endif
