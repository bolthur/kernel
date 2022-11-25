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

#include <libtar.h>

#ifndef _RAMDISK_H
#define _RAMDISK_H

#define MOUNT_POINT_DEVICE "/dev/ramdisk"
#define MOUNT_POINT_DESTINATION "/ramdisk"
#define MOUNT_POINT_FILESYSTEM "ramdisk"

void ramdisk_copy_from_shared( char* );
bool ramdisk_setup( void );
size_t ramdisk_get_size( const char* );
void* ramdisk_get_start( const char* );
TAR* ramdisk_get_info( const char* );

#endif
