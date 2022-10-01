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
  uint8_t jmp_code[ 3 ];
  char os_name[ 8 ];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t fat_table_count;
  uint16_t directory_entry_count;
  uint16_t sector_count_small;
  uint8_t media_type;
  uint16_t sectors_per_fat_small; // FAT12/16 only
  uint16_t sectors_per_track;
  uint16_t head_count;
  uint32_t hidden_sector_count;
  uint32_t sector_count_large;
  // extended boot record data
  union {
    // FAT12 / FAT16 extended boot record
    struct {
      uint8_t drive_number;
      uint8_t reserved;
      uint8_t signature;
      uint32_t serial;
      char label[ 11 ];
      char identifier[ 8 ];
      uint8_t boot_code[ 448 ];
    } ebr;
    // FAT32 extended boot record
    struct {
      uint32_t sectors_per_fat;
      uint16_t flags;
      uint16_t fat_version;
      uint32_t directory_cluster_count;
      uint16_t fsinfo_sector;
      uint16_t backup_boot_sector;
      uint8_t reserved[ 12 ];
      uint8_t drive_number;
      uint8_t flags_nt;
      uint8_t signature;
      uint32_t serial;
      char label[ 11 ];
      char identifier[ 8 ];
      uint8_t boot_code[ 420 ];
    } ebr32;
  };
  // signature
  uint8_t boot_sector_signature[ 2 ];
} fat_bpb_t;

static_assert( 512 == sizeof( fat_bpb_t ), "invalid fat_bpb_t size!" );

#pragma pack(pop)

#endif
