
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
#include <stdio.h>
#include <inttypes.h>

#include "list.h"
#include "vfs.h"

/**
 * @brief helper to destroy a node
 *
 * @param node
 */
static void vfs_destroy_node( vfs_node_ptr_t node ) {
  if ( ! node ) {
    return;
  }
  if ( node->pid ) {
    free( node->pid );
  }
  if ( node->name ) {
    free( node->name );
  }
  if ( node->children ) {
    list_destruct( node->children );
  }
  free( node );
}

/**
 * @brief Create and prepare vfs node
 *
 * @param pid
 * @param name
 * @param flags
 * @param parent
 * @return
 */
static vfs_node_ptr_t vfs_prepare_node(
  pid_t pid,
  const char* name,
  uint32_t flags,
  vfs_node_ptr_t parent
) {
  // allocate and error handling
  vfs_node_ptr_t node = malloc( sizeof( vfs_node_t ) );
  if ( ! node ) {
    return NULL;
  }
  // erase memory
  memset( node, 0, sizeof( vfs_node_t ) );
  // allocate name
  node->name = malloc( sizeof( char ) * ( strlen( name ) + 1 ) );
  if ( ! node->name ) {
    vfs_destroy_node( node );
    return NULL;
  }
  // allocate pid
  node->pid = malloc( sizeof( pid_t ) );
  if ( ! node->pid ) {
    vfs_destroy_node( node );
    return NULL;
  }
  // copy name, save current pid as handler and set flag to directory
  strcpy( node->name, name );
  *( node->pid ) = pid;
  node->flags = flags;
  // create list
  // FIXME: Add lookup and cleanup functions
  node->children = list_construct( NULL, NULL );
  if ( ! node->children ) {
    vfs_destroy_node( node );
    return NULL;
  }
  // append to parent
  if ( parent ) {
    // set parent
    node->parent = parent;
    // push dev item to root list
    if ( ! list_push_back( parent->children, node ) ) {
      vfs_destroy_node( node );
      return NULL;
    }
  }
  // return constructed node
  return node;
}

/**
 * @brief setup virtual file system
 *
 * @param current_pid
 * @return root node
 */
vfs_node_ptr_t vfs_setup( pid_t current_pid ) {
  // allocate root node
  vfs_node_ptr_t root = vfs_prepare_node( current_pid, "/", VFS_DIRECTORY, NULL );
  if ( ! root ) {
    return NULL;
  }
  // allocate dev node
  vfs_node_ptr_t dev = vfs_prepare_node( current_pid, "dev", VFS_DIRECTORY, root );
  if ( ! dev ) {
    vfs_destroy_node( root );
    return NULL;
  }
  // allocate ramdisk node
  vfs_node_ptr_t ramdisk = vfs_prepare_node( current_pid, "ramdisk", VFS_DIRECTORY, root );
  if ( ! ramdisk ) {
    vfs_destroy_node( dev );
    vfs_destroy_node( root );
    return NULL;
  }
  // return new vfs
  return root;
}

/**
 * @brief Method to dump complete vfs
 *
 * @param root
 * @param level
 */
void vfs_dump( vfs_node_ptr_t root, size_t level ) {
  if ( 0 < level ) {
    printf( "|" );
    for ( size_t i = 0; i < level * 2; i++ ) {
      printf( "-" );
    }
    printf( " " );
  }
  printf( "%s\r\n", root->name );

  // iterate children
  list_item_ptr_t node = root->children->first;
  while ( node ) {
    vfs_dump( ( vfs_node_ptr_t )node->data, level + 1 );
    node = node->next;
  }
}
