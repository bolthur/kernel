
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vfs.h"

/**
 * @brief setup virtual file system
 *
 * @param current_pid
 * @return root node
 */
vfs_node_ptr_t vfs_setup( pid_t current_pid ) {
  // allocate and error handling
  vfs_node_ptr_t root = malloc( sizeof( vfs_node_t ) );
  if ( ! root ) {
    return NULL;
  }
  // erase memory
  memset( root, 0, sizeof( vfs_node_t ) );
  // allocate name
  root->name = malloc( sizeof( char ) * ( strlen( "/" ) + 1 ) );
  if ( ! root->name ) {
    free( root );
    return NULL;
  }
  // allocate pid
  root->pid = malloc( sizeof( pid_t ) );
  if ( ! root->pid ) {
    free( root->name );
    free( root );
    return NULL;
  }
  // copy name, save current pid as handler and set flag to directory
  strcpy( root->name, "/" );
  *( root->pid ) = current_pid;
  root->flags = VFS_DIRECTORY;
  // return structure
  return root;
}

