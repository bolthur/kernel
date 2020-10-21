
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
#include <assert.h>
#include <avl.h>

/**
 * @brief Helper to create new tree
 *
 * @param compare compare function to be used within tree
 * @return avl_tree_ptr_t pointer to new tree
 */
avl_tree_ptr_t avl_create_tree( avl_compare_func_t compare ) {
  // allocate new tree structure
  avl_tree_ptr_t tree = ( avl_tree_ptr_t )malloc( sizeof( avl_tree_t ) );
  // assert malloc result
  assert( NULL != tree );
  // prepare structure
  memset( ( void* )tree, 0, sizeof( avl_tree_t ) );

  // fill structure itself
  tree->root = NULL;
  tree->compare = compare;

  // return created tree
  return tree;
}

/**
 * @brief creates and prepares a avl node
 *
 * @param data node data
 */
avl_node_ptr_t avl_create_node( void* data ) {
  // allocate node
  avl_node_ptr_t node = ( avl_node_ptr_t )malloc( sizeof( avl_node_t ) );
  // assert allocation
  assert( NULL != node );
  // prepare data
  memset( ( void* )node, 0, sizeof( avl_node_t ) );
  // call prepare node
  avl_prepare_node( node, data );
  // return created node
  return node;
}
