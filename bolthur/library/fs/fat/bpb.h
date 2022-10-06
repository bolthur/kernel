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
#include <assert.h>

#ifndef _FAT_BPB_H
#define _FAT_BPB_H

#pragma pack(push, 1)

typedef struct {
  uint8_t bios_drive_number;
  uint8_t reserved;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[ 11 ];
  char fat_type_label[ 8 ];
  uint8_t boot_code[ 448 ];
} fat_bpb_extended_fat_t;

typedef struct {
  uint32_t table_size_32;
  uint16_t extended_flags;
  uint16_t fat_version;
  uint32_t root_cluster;
  uint16_t fat_info;
  uint16_t backup_boot_sector;
  uint8_t reserved0[ 12 ];
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[ 11 ];
  char fat_type_label[ 8 ];
  uint8_t boot_code[ 420 ];
} fat_bpb_extended_fat32_t;

typedef struct {
  uint8_t bootjmp[ 3 ];
  char oem_name[ 8 ];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t table_count;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t table_size_16; // FAT12/16 only
  uint16_t sectors_per_track;
  uint16_t head_side_count;
  uint32_t hidden_sector_count;
  uint32_t total_sectors_32;
  // extended boot record data
  union {
    // FAT12 / FAT16 extended boot record
    fat_bpb_extended_fat_t fat;
    // FAT32 extended boot record
    fat_bpb_extended_fat32_t fat32;
    // raw data
    uint8_t raw[ 474 ];
  } extended;
  // signature
  uint16_t boot_sector_signature;
} fat_bpb_t;

static_assert( 512 == sizeof( fat_bpb_t ), "invalid fat_bpb_t size!" );

typedef struct {
  uint8_t bootjmp[ 3 ];
  char oem_name[ 8 ];
  uint8_t sbz[ 53 ];
  uint64_t partition_offset;
  uint64_t volume_length;
  uint32_t fat_offset;
  uint32_t fat_length;
  uint32_t cluster_heap_offset;
  uint32_t cluster_count;
  uint32_t root_dir_cluster;
  uint32_t serial_number;
  uint16_t revision;
  uint16_t flags;
  uint8_t sector_shift;
  uint8_t cluster_shift;
  uint8_t fat_count;
  uint8_t selected_drive;
  uint8_t percentage_used;
  uint8_t reserved[ 7 ];
} fat_bpb_extended_t;

static_assert(
  120 == sizeof( fat_bpb_extended_t ),
  "invalid fat_bpb_extended_t size!"
);

#pragma pack(pop)

#endif
