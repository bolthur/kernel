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

#if ! defined( _FAT_H )
#define _FAT_H

// fat bios parameter block
typedef struct {
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
} fat_bpb_t;

// fat32 fs info
typedef struct fat32_fsinfo {
  uint32_t lead_signature;
  uint8_t reserved[ 480 ];
  uint32_t signature;
  uint32_t known_free_cluster;
  uint32_t available_cluster_start;
  uint8_t reserved2[ 12 ];
  uint32_t trail_signature;
} fat32_fsinfo_t;

#endif
