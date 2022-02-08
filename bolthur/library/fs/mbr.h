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

#if ! defined( _MBR_H )
#define _MBR_H

#define PARTITION_TYPE_FAT12_CHS 0x01
#define PARTITION_TYPE_FAT16_CHS 0x04
#define PARTITION_TYPE_FAT16B_CHS 0x06
#define PARTITION_TYPE_NTFS 0x07
#define PARTITION_TYPE_EXFAT 0x07
#define PARTITION_TYPE_FAT32_CHS 0x0B
#define PARTITION_TYPE_FAT32_LBA 0x0C
#define PARTITION_TYPE_FAT16B_LBA 0x0E
// linux native file systems like ext2, ext3, ext4, ...
#define PARTITION_TYPE_LINUX_NATIVE 0x83

struct mbr_table_entry {
  union {
    uint32_t raw[ 4 ];
    struct {
      uint8_t bootable;
      uint8_t start_head;
      uint16_t start_sector : 6;
      uint16_t start_cylinder : 10;
      uint8_t system_id;
      uint8_t end_head;
      uint16_t end_sector : 6;
      uint16_t end_cylinder : 10;
      uint32_t relative_sector;
      uint32_t total_sector;
    } data;
  };
};
typedef struct mbr_table_entry mbr_table_entry_t;
typedef struct mbr_table_entry* mbr_table_entry_ptr_t;

struct mbr {
  uint8_t unused[ 446 ];
  mbr_table_entry_t partition_table[ 4 ];
  uint8_t signatur[ 2 ];
} __packed;
typedef struct mbr mbr_t;
typedef struct mbr* mbr_ptr_t;

#endif
