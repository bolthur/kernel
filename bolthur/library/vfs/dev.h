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

#ifndef _LIBVFS_DEV_H
#define _LIBVFS_DEV_H

#include <stdint.h>
#include <stddef.h>
#include <sys/bolthur.h>

bool vfs_dev_add_file( const char*, const uint32_t*, size_t, rpc_handler_t );
bool vfs_dev_add_folder( const char*, const uint32_t*, size_t, rpc_handler_t );
bool vfs_dev_add_folder_file_stat( const char*, const struct stat*, rpc_handler_t );

#endif
