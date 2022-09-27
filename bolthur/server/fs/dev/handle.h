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

#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../../../library/collection/list/list.h"

#ifndef _HANDLE_H
#define _HANDLE_H

typedef struct {
  char* path;
  struct stat info;
  pid_t process;
} device_handle_t;

bool handle_init( void );
device_handle_t* handle_get_by_path( const char* );
device_handle_t* handle_get_by_id( pid_t );
bool handle_add( const char*, struct stat, pid_t );

#endif
