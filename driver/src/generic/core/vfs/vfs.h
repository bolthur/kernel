
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

// exclusive vfs types
#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02
#define VFS_CHARDEVICE 0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE 0x05
#define VFS_SYMLINK 0x06
#define VFS_HARDLINK 0x07
// possible combinations for exclusive types
#define VFS_MOUNTPOINT 0x10

// forward declaration necessary due to circular referencing
typedef struct vfs_node vfs_node_t,*vfs_node_ptr_t;
// structure itself
typedef struct vfs_node {
  char *name;
  uint32_t permission_mask;
  uint32_t user_id;
  uint32_t group_id;
  uint32_t flags;
  uint32_t length;
  vfs_node_ptr_t target;
  pid_t pid;
  list_manager_ptr_t children;
  list_manager_ptr_t handle;
} vfs_node_t, *vfs_node_ptr_t;

// functions
vfs_node_ptr_t vfs_setup( pid_t );
void vfs_destroy( vfs_node_ptr_t );
void vfs_dump( vfs_node_ptr_t, const char* );
bool vfs_add_path( vfs_node_ptr_t, pid_t, const char*, vfs_entry_type_t );

vfs_node_ptr_t vfs_node_by_name( vfs_node_ptr_t, const char* );
vfs_node_ptr_t vfs_node_by_path( const char* );

#endif
