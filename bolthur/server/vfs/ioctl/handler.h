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
#include "../collection/avl.h"
#include "../vfs.h"
#include "../file/handle.h"

#if !defined( _IOCTL_HANDLER_H )
#define _IOCTL_HANDLER_H

struct ioctl_tree_entry {
  avl_node_t node;
  pid_t pid;
  avl_tree_ptr_t tree;
};
typedef struct ioctl_tree_entry ioctl_tree_entry_t;
typedef struct ioctl_tree_entry *ioctl_tree_entry_ptr_t;

struct ioctl_container {
  avl_node_t node;
  uint32_t command;
};
typedef struct ioctl_container ioctl_container_t;
typedef struct ioctl_container *ioctl_container_ptr_t;

#define IOCTL_HANDLER_GET_ENTRY( n ) \
  ( ioctl_tree_entry_ptr_t )( ( uint8_t* )n - offsetof( ioctl_tree_entry_t, node ) )

#define IOCTL_HANDLER_GET_CONTAINER( n ) \
  ( ioctl_container_ptr_t )( ( uint8_t* )n - offsetof( ioctl_container_t, node ) )

bool ioctl_handler_init( void );
ioctl_container_ptr_t ioctl_lookup_command( uint32_t, pid_t );
bool ioctl_push_command( uint32_t, pid_t );

#endif
