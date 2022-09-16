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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/bolthur.h>

#if ! defined( _LIBMOUNT_H )
#define _LIBMOUNT_H

#define MOUNT_MOUNT RPC_CUSTOM_START
#define MOUNT_AUTO_MOUNT MOUNT_MOUNT + 1
#define MOUNT_UNMOUNT MOUNT_AUTO_MOUNT + 1

typedef struct {
  char device[ PATH_MAX ];
  char mount_point[ PATH_MAX ];
  char fs_type[ 100 ];
} mount_mount_t;

typedef struct {
  char device[ PATH_MAX ];
  char mount_point[ PATH_MAX ];
} mount_auto_mount_t;

typedef struct {
  char mount_point[ PATH_MAX ];
} mount_unmount_t;

#endif
