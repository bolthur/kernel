/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
  [[maybe_unused]] static inline
)

/**
 * @fn int handler_cmp(const struct handler_node*, const struct handler_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
int handler_cmp( const struct handler_node* a, const struct handler_node* b ) {
  return a->process == b->process ? 0 : ( a->process > b->process ) ? 1 : -1;
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
 * @param process
 * @param create
 * @return
 */
handler_node_t* handler_extract( const pid_t process, const bool create ) {
  // allocate node
  handler_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return NULL;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->process = process;
  // lookup for node
  handler_node_t* found = handler_node_tree_find( &management_tree, node );
  // insert if not existing
  if ( ! found ) {
    if ( ! create ) {
      free( node );
      return NULL;
    }
    if ( handler_node_tree_insert( &management_tree, node ) ) {
      free( node );
      return NULL;
    }
    return handler_node_tree_find( &management_tree, node );
  }
  // free up again
  free( node );
  // return found
  return found;
}

/**
 * @fn int handler_add(console_t*, pid_t)
 * @brief Helper to add a watch node
 *
 * @param console
 * @param process
 * @return
 */
int handler_add( console_t* console, pid_t process ) {
  // ensure that it doesn't exist
  handler_node_t* node = handler_extract( process, false );
  if ( node ) {
    return -EEXIST;
  }
  // try to create it
  node = handler_extract( process, true );
  if ( ! node ) {
    return -ENOMEM;
  }
  // allocate handler if not allocated
  if ( ! node->console ) {
    // duplicate string
    node->console = console;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_remove(pid_t)
 * @brief Helper to remove a handler node
 *
 * @param process
 * @return
 */
int handler_remove( const pid_t process ) {
  handler_node_t* node = handler_extract( process, false );
  if ( ! node ) {
    return 0;
  }
  // remove node tree
  handler_node_tree_remove( &management_tree, node );
  // free node
  free( node );
  // return success
  return 0;
}
