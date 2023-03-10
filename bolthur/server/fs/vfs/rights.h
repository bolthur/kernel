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

#include <sys/bolthur.h>

#ifndef _RIGHTS_H
#define _RIGHTS_H

typedef struct rights_check_context rights_check_context_t;

typedef void ( *rights_handler_t )( rights_check_context_t*, bolthur_async_data_t* );

typedef struct rights_check_context {
  char* path;
  size_t type;
  pid_t origin;
  size_t data_info;
  rights_handler_t callback;
  size_t request_size;
  void* request;
  vfs_stat_response_t* file_stat;
  vfs_stat_response_t* authenticate_stat;
  vfs_ioctl_perform_response_t* rights;
  size_t rights_size;
} rights_check_context_t;

void rights_handle_permission( bolthur_async_data_t*, vfs_ioctl_perform_response_t*, size_t );
void rights_handle_file_stat( bolthur_async_data_t*, vfs_stat_response_t*, rpc_handler_t );
void rights_handle_authenticate_stat( bolthur_async_data_t*, vfs_stat_response_t*, rpc_handler_t );
void rights_check( const char*, rights_handler_t, rpc_handler_t, void*, size_t, size_t, pid_t, size_t );

#endif
