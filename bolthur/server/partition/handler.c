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

#include <errno.h>
#include <libgen.h>
#include "handler.h"

// define tree
HANDLER_TREE_DEFINE(
  handler_tree,
  handler_node,
  node,
  handler_cmp,
  __unused static inline
)

/**
 * @fn int handler_cmp(struct handler_node*, struct handler_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
int handler_cmp(
  struct handler_node* a,
  struct handler_node* b
) {
  return strcmp( a->name, b->name );
}

// create static tree
static struct handler_tree management_tree;

/**
 * @fn bool handler_setup(void)
 * @brief mountpoint node setup
 *
 * @return
 */
bool handler_setup( void ) {
  handler_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn handler_node_t handler_extract*(const char*,bool)
 * @brief Extract watch information by name
 *
 * @param name
 * @param create
 * @return
 */
handler_node_t* handler_extract( const char* name, bool create ) {
  // allocate node
  handler_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return NULL;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->name = strdup( name );
  if ( ! node->name ) {
    free( node );
    return NULL;
  }
  // lookup for node
  handler_node_t* found = handler_node_tree_find( &management_tree, node );
  // insert if not existing
  if ( ! found ) {
    if ( ! create ) {
      free( node->name );
      free( node );
      return NULL;
    }
    if ( handler_node_tree_insert( &management_tree, node ) ) {
      free( node->name );
      free( node );
      return NULL;
    }
    return handler_node_tree_find( &management_tree, node );
  }
  // free up again
  free( node->name );
  free( node );
  // return found
  return found;
}

/**
 * @fn int handler_add(const char*, const char*, int)
 * @brief Helper to add a watch node
 *
 * @param filesystem
 * @param handler
 * @param fd
 * @return
 */
int handler_add( const char* filesystem, pid_t handler ) {
  // ensure that it doesn't exists
  handler_node_t* node = handler_extract( filesystem, false );
  if ( node ) {
    return -EEXIST;
  }
  // try to create it
  node = handler_extract( filesystem, true );
  if ( ! node ) {
    return -ENOMEM;
  }
  // allocate handler if not allocated
  if ( ! node->handler ) {
    // duplicate string
    node->handler = handler;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_remove(const char*)
 * @brief Helper to add a watch node
 *
 * @param path
 * @param handler
 * @return
 */
int handler_remove( const char* filesystem ) {
  handler_node_t* node = handler_extract( filesystem, false );
  if ( ! node ) {
    return 0;
  }
  // remove node tree
  handler_node_tree_remove( &management_tree, node );
  // free up data
  if ( node->name ) {
    free( node->name );
  }
  free( node );
  // return success
  return 0;
}

/**
 * @fn void handler_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void handler_dump( void ) {
  EARLY_STARTUP_PRINT( "mountpoint node tree dump\r\n" )
  handler_tree_each(&management_tree, handler_node, n, {
      EARLY_STARTUP_PRINT("%s\r\n", n->name);
  });
}
