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

#include "../../stdlib.h"
#include "../../string.h"
#include "../avl.h"

/**
 * @brief Default lookup if not passed during creation
 *
 * @param a
 * @param b
 * @return int32_t
 */
int32_t avl_default_lookup(
  const avl_node_ptr_t a,
  const void* b
) {
  if ( a->data == b ) {
    return 0;
  }

  return a->data > b
    ? -1
    : 1;
}

/**
 * @brief Default cleanup if not passed during creation
 *
 * @param a
 */
void avl_default_cleanup( __unused const avl_node_ptr_t a ) {
}

/**
 * @brief Helper to create new tree
 *
 * @param compare compare function to be used within tree
 * @param lookup
 * @param cleanup
 * @return avl_tree_ptr_t pointer to new tree
 */
avl_tree_ptr_t avl_create_tree(
  avl_compare_func_t compare,
  avl_lookup_func_t lookup,
  avl_cleanup_func_t cleanup
) {
  // allocate new tree structure
  avl_tree_ptr_t new_tree = ( avl_tree_ptr_t )malloc( sizeof( avl_tree_t ) );
  // check malloc return
  if ( !new_tree ) {
    return NULL;
  }
  // prepare structure
  memset( ( void* )new_tree, 0, sizeof( avl_tree_t ) );

  // fill structure itself
  new_tree->root = NULL;
  new_tree->compare = compare;
  // lookup function
  if( lookup ) {
    new_tree->lookup = lookup;
  } else {
    new_tree->lookup = avl_default_lookup;
  }
  // cleanup function
  if( cleanup ) {
    new_tree->cleanup = cleanup;
  } else {
    new_tree->cleanup = avl_default_cleanup;
  }

  // return created tree
  return new_tree;
}

/**
 * @brief creates and prepares a avl node
 *
 * @param data node data
 */
avl_node_ptr_t avl_create_node( void* data ) {
  // allocate node
  avl_node_ptr_t node = ( avl_node_ptr_t )malloc( sizeof( avl_node_t ) );
  // check malloc return
  if ( ! node ) {
    return NULL;
  }
  // prepare data
  memset( ( void* )node, 0, sizeof( avl_node_t ) );
  // call prepare node
  avl_prepare_node( node, data );
  // return created node
  return node;
}
