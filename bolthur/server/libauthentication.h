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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/bolthur.h>

#if ! defined( _LIBDEV_H )
#define _LIBDEV_H

#define AUTHENTICATE_REQUEST RPC_CUSTOM_START
#define AUTHENTICATE_FETCH AUTHENTICATE_REQUEST + 1

typedef struct {
  char user[ PATH_MAX ];
  char password[ PATH_MAX ];
  pid_t process;
} authentication_request_request_t;

typedef struct {
  pid_t process;
} authentication_fetch_request_t;

typedef struct {
  uid_t uid;
  size_t group_count;
  gid_t gid[];
} authentication_fetch_response_t;

#endif
