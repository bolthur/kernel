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

#ifndef _LIBAUTHENTICATION_H
#define _LIBAUTHENTICATION_H

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/bolthur.h>

#define AUTHENTICATE_REQUEST RPC_CUSTOM_START
#define AUTHENTICATE_FETCH AUTHENTICATE_REQUEST + 1
#define AUTHENTICATE_RELOAD AUTHENTICATE_FETCH + 1

typedef struct {
  size_t shm_id;
  pid_t process;
} authentication_request_request_t;

typedef struct {
  char user[ PATH_MAX ];
  char password[ PATH_MAX ];
  // used for return
  char pw_user[ PATH_MAX ];
  char pw_home[ PATH_MAX ];
  char pw_shell[ PATH_MAX ];
} authentication_request_request_data_t;

typedef struct {
  pid_t process;
} authentication_fetch_request_t;

typedef struct {
  uid_t uid;
  size_t group_count;
  gid_t gid[];
} authentication_fetch_response_t;

typedef struct {
  pid_t process;
} authenticate_reload_request_t;

typedef struct {
  int result;
} authenticate_reload_response_t;

#endif
