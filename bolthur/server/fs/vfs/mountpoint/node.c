/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <libgen.h>
#include "node.h"

/**
 * @fn int mountpoint_cmp(struct mountpoint_node*, struct mountpoint_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
static int mountpoint_cmp(
  struct mountpoint_node* a,
  struct mountpoint_node* b
) {
  return strcmp( a->name, b->name );
}

// define tree
MOUNTPOINT_TREE_DEFINE(
  mountpoint_tree,
  mountpoint_node,
  node,
  mountpoint_cmp,
  __unused static inline
)
// create static tree
static struct mountpoint_tree management_tree;

/**
 * @fn bool mountpoint_node_setup(void)
 * @brief mountpoint node setup
 *
 * @return
 */
bool mountpoint_node_setup( void ) {
  mountpoint_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn mountpoint_node_t mountpoint_node_extract*(const char*)
 * @brief Extract mount point node by name
 *
 * @param name
 * @return
 */
mountpoint_node_t* mountpoint_node_extract( const char* path ) {
  // allocate node
  mountpoint_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->name = strdup( path );
  if ( ! node->name ) {
    free( node );
    return false;
  }

  // local pointer for path
  char* p = ( char* )path;
  // set loop path and found
  char* loop_path = p;
  mountpoint_node_t* found = NULL;
  // try to get mount point
  while ( ! found && *loop_path ) {
    // lookup
    found = mountpoint_node_tree_find( &management_tree, node );
    // handle not found ( use basename next )
    if ( ! found ) {
      char* tmp = dirname( loop_path );
      // handle null or no change
      if ( ! tmp || tmp == loop_path ) {
        break;
      }
      // handle loop path not equal to p
      if ( loop_path != p ) {
        free( loop_path );
      }
      // overwrite loop_path
      loop_path = tmp;
      // clear out
      memset( node->name, 0, strlen( path ) );
      strcpy( node->name, loop_path );
    }
  }
  // free loop path
  if ( loop_path != p ) {
    free( loop_path );
  }
  free( node->name );
  free( node );
  // handle found
  return found;
}

/**
 * @fn bool mountpoint_node_add(const char*, pid_t, struct stat*)
 * @brief Helper to add a mountpoint node
 *
 * @param path
 * @param handler
 * @param st
 * @return
 */
bool mountpoint_node_add( const char* path, pid_t handler, struct stat* st ) {
  // allocate node
  mountpoint_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->name = strdup( path );
  if ( ! node->name ) {
    free( node );
    return false;
  }
  // copy over stat result
  if ( st ) {
    node->st = malloc( sizeof( struct stat ) );
    if ( ! node->st ) {
      free( node->name );
      free( node );
      return false;
    }
    memcpy( node->st, st, sizeof( struct stat ) );
  }
  // set responsible process
  node->pid = handler;
  // handle already existing and insert
  if (
    mountpoint_node_tree_find( &management_tree, node )
    || mountpoint_node_tree_insert( &management_tree, node )
  ) {
    if ( node->st ) {
      free( node->st );
    }
    free( node->name );
    free( node );
    return false;
  }
  return true;
}

/**
 * @fn void mountpoint_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void mountpoint_node_dump( void ) {
  EARLY_STARTUP_PRINT( "mountpoint node tree dump\r\n" )
  mountpoint_node_tree_each(&management_tree, mountpoint_node, n, {
      EARLY_STARTUP_PRINT("%s\r\n", n->name);
  });
}