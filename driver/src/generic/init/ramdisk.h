/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <libtar.h>

#if !defined( __RAMDISK_H__ )
#define __RAMDISK_H__

extern uintptr_t ramdisk_compressed;
extern size_t ramdisk_compressed_size;
extern uintptr_t ramdisk_decompressed;
extern size_t ramdisk_decompressed_size;
extern size_t ramdisk_read_offset;

size_t ramdisk_extract_size( uintptr_t, size_t );
void* ramdisk_extract( uintptr_t, size_t, size_t );
void* ramdisk_lookup_file( TAR*, const char*, size_t* );
void ramdisk_dump( TAR* );

#endif
