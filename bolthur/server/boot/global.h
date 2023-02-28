/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <stddef.h>
#include <libtar.h>

#ifndef _GLOBAL_H
#define _GLOBAL_H

extern size_t ramdisk_shared_id;
extern TAR *disk;
extern int fd_dev_manager;
extern char* bootargs;

extern pid_t own_pid;
extern pid_t vfs_pid;
extern pid_t dev_pid;
extern pid_t ramdisk_pid;

#endif
