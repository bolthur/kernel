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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "process.h"

/**
 * @fn int process_cmp(struct process_node*, struct process_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
static int process_cmp(
  struct process_node* a,
  struct process_node* b
) {
  if ( a->pid == b->pid ) {
    return 0;
  }
  return a->pid > b->pid ? 1 : -1;
}

// define tree
PROCESS_TREE_DEFINE(
  process_tree,
  process_node,
  node,
  process_cmp,
  __unused static inline
)

// create static tree
static struct process_tree management_tree;

/**
 * @fn bool process_setup(void)
 * @brief Setup process handling
 *
 * @return
 */
bool process_setup( void ) {
  process_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn process_node_t process_generate*(pid_t)
 * @brief Generate process structure if necessary
 *
 * @param proc
 * @return
 */
process_node_t* process_generate( pid_t proc ) {
  // allocate node
  process_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return NULL;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate
  node->pid = proc;
  // try to find one and return if existing
  process_node_t* found = process_node_tree_find( &management_tree, node );
  if ( found ) {
    free( node );
    return found;
  }
  // further setup
  node->handle = 3;
  handle_node_tree_init(&node->management_tree);
  // handle already existing and insert
  if ( process_node_tree_insert( &management_tree, node ) ) {
    free( node );
    return NULL;
  }
  return node;
}

/**
 * @fn void process_remove(process_node_t*)
 * @brief Delete a node tree
 *
 * @param node
 */
void process_remove( process_node_t* node ) {
  process_node_tree_remove( &management_tree, node );
  free( node );
}

/**
 * @fn int process_duplicate(process_node_t*, handle_node_t*)
 * @brief Method to duplicate a given handle
 *
 * @param new_container
 * @param handle
 * @return
 */
int process_duplicate( process_node_t* new_container, handle_node_t* handle ) {
  handle_node_t* new_handle = malloc( sizeof( *new_handle ) );
  if ( ! new_handle ) {
    return -ENOMEM;
  }
  // copy over contents
  memcpy( new_handle, handle, sizeof( *new_handle ) );
  // try to add to new container tree
  if ( handle_node_tree_insert( &new_container->management_tree, new_handle ) ) {
    free( new_handle );
    return -EEXIST;
  }
  // return success
  return 0;
}

