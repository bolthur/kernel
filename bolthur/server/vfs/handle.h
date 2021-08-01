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

#include <sys/syslimits.h>
#include <sys/types.h>
#include "avl.h"
#include "vfs.h"

#if !defined( __HANDLE_H__ )
#define __HANDLE_H__

struct handle_pid {
  avl_node_t node;
  pid_t pid;
  int handle;
  avl_tree_ptr_t tree;
};

struct handle_container {
  avl_node_t node;
  int handle;
  int flags;
  int mode;
  off_t pos;
  char path[ PATH_MAX ];
  vfs_node_ptr_t target;
};

typedef struct handle_pid handle_pid_t;
typedef struct handle_pid *handle_pid_ptr_t;
typedef struct handle_container handle_container_t;
typedef struct handle_container *handle_container_ptr_t;

#define HANDLE_GET_CONTAINER( n ) \
  ( handle_container_ptr_t )( ( uint8_t* )n - offsetof( handle_container_t, node ) )

#define HANDLE_GET_PID( n ) \
  ( handle_pid_ptr_t )( ( uint8_t* )n - offsetof( handle_pid_t, node ) )

void handle_init( void );
int handle_generate( handle_container_ptr_t*, pid_t, vfs_node_ptr_t, vfs_node_ptr_t, const char*, int, int );
int handle_destory( pid_t, int );
int handle_get( handle_container_ptr_t*, pid_t, int );

#endif
