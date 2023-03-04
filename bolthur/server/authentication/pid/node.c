/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
 * @fn int pid_cmp(struct pid_node*, struct pid_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
static int pid_cmp(
  struct pid_node* a,
  struct pid_node* b
) {
  if ( a->pid == b->pid ) {
    return 0;
  }
  return a->pid > b->pid ? 1 : -1;
}

// define tree
PID_TREE_DEFINE(
  pid_tree,
  pid_node,
  node,
  pid_cmp,
  __unused static inline
)
// create static tree
static struct pid_tree management_tree;

/**
 * @fn bool pid_node_setup(void)
 * @brief pid node setup
 *
 * @return
 */
bool pid_node_setup( void ) {
  pid_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn pid_node_t pid_node_extract*(pid_t)
 * @brief Extract node
 *
 * @param name
 * @return
 */
pid_node_t* pid_node_extract( pid_t pid ) {
  // allocate node
  pid_node_t node;
  // clear out node
  memset( &node, 0, sizeof( node ) );
  // populate
  node.pid = pid;
  // return lookup
  return pid_node_tree_find( &management_tree, &node );
}

/**
 * @fn void pid_node_remove(pid_t)
 * @brief Method to remove a node
 *
 * @param path
 */
void pid_node_remove( pid_t pid ) {
  pid_node_t* node = pid_node_extract( pid );
  if ( ! node ) {
    return;
  }
  // remove node tree
  pid_node_tree_remove( &management_tree, node );
  // free node
  free( node );
}

/**
 * @fn bool pid_node_add(pid_t, uid_t)
 * @brief Helper to add a new node
 *
 * @param path
 * @param handler
 * @param st
 * @return
 */
bool pid_node_add( pid_t pid, uid_t user ) {
  // allocate node
  pid_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate
  node->pid = pid;
  node->uid = user;
  // handle already existing and insert
  if (
    pid_node_tree_find( &management_tree, node )
    || pid_node_tree_insert( &management_tree, node )
  ) {
    free( node );
    return false;
  }
  return true;
}

/**
 * @fn void pid_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void pid_node_dump( void ) {
  EARLY_STARTUP_PRINT( "pid node tree dump\r\n" )
  pid_node_tree_each(&management_tree, pid_node, n, {
    EARLY_STARTUP_PRINT( "%d | %d\r\n", n->pid, n->uid )
  });
}
