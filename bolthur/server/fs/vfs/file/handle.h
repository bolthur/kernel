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

#include <sys/syslimits.h>
#include <sys/types.h>
#include "../mountpoint/node.h"
#include "../../../../library/collection/avl/avl.h"

#ifndef _FILE_HANDLE_H
#define _FILE_HANDLE_H

typedef struct {
  avl_node_t node;
  pid_t pid;
  int handle;
  avl_tree_t* tree;
} handle_pid_t;

typedef struct {
  avl_node_t node;
  int handle;
  int flags;
  int mode;
  off_t pos;
  char path[ PATH_MAX ];
  mountpoint_node_t* mount_point;
  pid_t handler;
  struct stat info;
} handle_container_t;

#define HANDLE_GET_CONTAINER( n ) \
  ( handle_container_t* )( ( uint8_t* )n - offsetof( handle_container_t, node ) )

#define HANDLE_GET_PID( n ) \
  ( handle_pid_t* )( ( uint8_t* )n - offsetof( handle_pid_t, node ) )

#define process_handle_for_each( iter, item, tree ) \
  for ( \
    iter = avl_iterate_first( tree ), \
    item = HANDLE_GET_CONTAINER( iter ); \
    iter != NULL && item != NULL; \
    iter = avl_iterate_next( tree, iter ), \
    item = HANDLE_GET_CONTAINER( iter ) \
  )

bool handle_init( void );
int handle_generate( handle_container_t**, pid_t, pid_t, mountpoint_node_t*, const char*, int, int );
int handle_destory( pid_t, int );
void handle_destory_all( pid_t );
int handle_get( handle_container_t**, pid_t, int );
handle_pid_t* handle_get_process_container( pid_t );
handle_pid_t* handle_generate_container( pid_t );
bool handle_duplicate( handle_container_t*, handle_pid_t* );

#endif
