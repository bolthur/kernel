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

#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <sys/types.h>
#include <ext2fs/ext2fs.h>

#define SECTOR_SIZE 512
#define TOTAL_IMAGE_SIZE ( 1024 * 1024 * 1024 / SECTOR_SIZE )

#define FAT_PARTITION_START 2048
#define FAT_PARTITION_SIZE ( 256 * 1024 * 1024 / SECTOR_SIZE )
#define EXT_PARTITION_START ( FAT_PARTITION_START + FAT_PARTITION_SIZE )
#define EXT_PARTITION_SIZE ( TOTAL_IMAGE_SIZE - EXT_PARTITION_START )

extern FILE* global_sd_image_fd;
extern struct struct_io_manager custom_io_manager;

bool copy_file_to_boot( const char* path, const char* destination );
bool copy_file_to_root( const char* path, const char* destination );
bool link_file_in_root( const char* target, const char* destination );

bool create_empty_image( const char* path );

bool copy_firmware_to_boot_image( void );
bool copy_ramdisk_to_boot_image( void );
bool copy_kernel_to_boot_image( void );;

bool populate_boot_image( void );
bool populate_root_image( void );

#endif
