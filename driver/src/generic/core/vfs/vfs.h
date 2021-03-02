
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

#if !defined( __VFS_H__ )
#define __VFS_H__

#include <stdint.h>
#include <unistd.h>
#include "list.h"

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02
#define VFS_CHARDEVICE 0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE 0x05
#define VFS_SYMLINK 0x06

#define VFS_MOUNTPOINT 0x10

typedef struct vfs_node vfs_node_t,*vfs_node_ptr_t;

/**
 * @struct VFS node structure
 * Structure representing one entry in virtual file system
 */
typedef struct vfs_node {
  char *name; /**< entry name */
  uint32_t permission_mask; /**< permission mask */
  uint32_t user_id; /**< user id */
  uint32_t group_id; /**< group id */
  uint32_t flags; /**< flags ( including node type ) */
  uint32_t length; /**< byte size of file */
  vfs_node_ptr_t target; /**< target link ( symlink or mountpoint ) */
  pid_t *pid; /**< process taking care of the node ( check parent if NULL ) */
  list_manager_ptr_t children; /**< list of child nodes */
  vfs_node_ptr_t parent; /**< parent node */
} vfs_node_t, *vfs_node_ptr_t;

vfs_node_ptr_t vfs_setup( pid_t );
void vfs_destroy( vfs_node_ptr_t );
void vfs_dump( vfs_node_ptr_t, const char* );
bool vfs_add_path( vfs_node_ptr_t, pid_t, const char*, bool );

#endif
