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

#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "list.h"

#if !defined( __VFS_H__ )
#define __VFS_H__

// forward declaration necessary due to circular referencing
typedef struct vfs_node vfs_node_t;
typedef struct vfs_node *vfs_node_ptr_t;

// structure itself
struct vfs_node {
  pid_t pid;
  char *name;
  char* target;
  struct stat* st;
  list_manager_ptr_t children;
  list_manager_ptr_t handle;
  vfs_node_ptr_t parent;
};

// functions
vfs_node_ptr_t vfs_setup( pid_t );
void vfs_destroy( vfs_node_ptr_t );
void vfs_dump( vfs_node_ptr_t, const char* );
bool vfs_add_path( vfs_node_ptr_t, pid_t, const char*, char*, struct stat* );
vfs_node_ptr_t vfs_node_by_name( vfs_node_ptr_t, const char* );
vfs_node_ptr_t vfs_node_by_path( const char* );
char* vfs_path_bottom_up( vfs_node_ptr_t );

#endif
