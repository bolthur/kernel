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

#include <inttypes.h>
#include <libgen.h>
#include "node.h"
#include "global.h"

/**
 * @fn int handler_cmp(struct handler_node*, struct handler_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
static int handler_cmp(
  const struct handler_node* a,
  const struct handler_node* b
) {
  if ( a->type == b->type ) {
    return 0;
  }
  return a->type > b->type ? 1 : -1;
}

// define tree
HANDLER_TREE_DEFINE(
  handler_tree,
  handler_node,
  node,
  handler_cmp,
  [[maybe_unused]] static inline
)
// create static tree
static struct handler_tree management_tree;

/**
 * @fn bool handler_node_setup(void)
 * @brief handler node setup
 *
 * @return
 */
bool handler_node_setup( void ) {
  handler_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn handler_node_t handler_node_extract*(uint32_t)
 * @brief Extract mount point node by name
 *
 * @param type
 * @return
 */
handler_node_t* handler_node_extract( const uint32_t type ) {
  handler_node_t node = { .type = type, };
  return handler_node_tree_find( &management_tree, &node );
}

/**
 * @fn void handler_node_remove(uint32_t)
 * @brief Method to remove a handler type
 *
 * @param type
 */
void handler_node_remove( const uint32_t type ) {
  handler_node_t* node = handler_node_extract( type );
  if ( ! node ) {
    return;
  }
  // remove node tree
  handler_node_tree_remove( &management_tree, node );
  // free up data
  free( node );
}

/**
 * @fn bool handler_node_add(uint32_t, pid_t)
 * @brief Helper to add a handler node
 *
 * @param type
 * @param handler
 * @return
 */
bool handler_node_add( const uint32_t type, const pid_t handler ) {
  // allocate node
  handler_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate
  node->type = type;
  node->handler = handler;
  // handle already existing and insert
  if (
    handler_node_tree_find( &management_tree, node )
    || handler_node_tree_insert( &management_tree, node )
  ) {
    free( node );
    return false;
  }
  return true;
}

/**
 * @fn void handler_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void handler_node_dump( void ) {
  #if defined( VFS_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "handler node tree dump\r\n" )
    handler_node_tree_each(&management_tree, handler_node, n, {
        EARLY_STARTUP_PRINT("%"PRIu32" / %d\r\n", n->type, n->handler);
    });
  #endif
}
