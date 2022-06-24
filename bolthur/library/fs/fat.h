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

#ifndef _FAT_H
#define _FAT_H

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

// fat bios parameter block
typedef union __packed {
  uint8_t plain[ 533 ];
  struct {
    uint8_t jmp_code[ 3 ];
    char os_name[ 8 ];
    uint16_t sector_size;
    uint8_t sector_per_cluster;
    uint16_t sector_reserved;
    uint8_t num_fat_table;
    uint16_t root_directory_count;
    uint16_t total_sector;
    uint8_t media_type;
    uint16_t sector_per_fat; // FAT12/16 only
    uint16_t sector_per_track;
    uint16_t head;
    uint32_t sector_hidden;
    uint32_t total_sector_large;
    // extended boot record data
    union {
      // FAT12 / FAT16 extended boot record
      struct {
        uint8_t drive;
        uint8_t reserved;
        uint8_t signature;
        uint32_t serial;
        char label[ 11 ];
        char identifier[ 8 ];
        uint8_t unused[ 448 ];
      } ebr;
      // FAT32 extended boot record
      struct {
        uint32_t sector_per_fat;
        uint16_t flag;
        uint16_t fat_version;
        uint32_t root_directory_cluster_count;
        uint16_t fsinfo_sector;
        uint16_t backup_bpb_sector;
        uint8_t reserved[ 12 ];
        uint8_t drive;
        uint8_t nt_flag;
        uint8_t signature;
        uint32_t serial;
        char label[ 11 ];
        char identifier[ 8 ];
        uint8_t unused[ 420 ];
      } ebr32;
      // placeholder
      uint8_t unused[ 474 ];
    };
    uint8_t partition_signature[ 2 ];
  } data;
} fat_bpb_t;

// fat32 fs info
typedef union __packed {
  uint8_t plain[ 512 ];
  struct {
    uint32_t lead_signature;
    uint8_t reserved[ 480 ];
    uint32_t signature;
    uint32_t known_free_cluster;
    uint32_t available_cluster_start;
    uint8_t reserved2[ 12 ];
    uint32_t trail_signature;
  } data;
} fat32_fsinfo_t;

// fat directory
typedef union __packed {
  char plain[ 32 ];
  struct {
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
  } data;
} fat_directory_t;

// long file name structure
typedef struct __packed {
  uint8_t order;
  char first_five[ 10 ];
  uint8_t attributes;
  uint8_t checksum;
  char next_six[ 12 ];
  uint16_t zero;
  char final_two[ 4 ];
} fat_long_file_name_t;

#endif
