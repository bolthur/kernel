
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

#include <avl.h>
#include <assert.h>

/**
 * @brief Internal function for inserting a node
 *
 * @param tree
 * @param node
 * @param root
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t insert(
  avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t root
) {
  // handle empty root
  if ( NULL == root ) {
    return node;
  }

  int32_t result = tree->compare( root, node );

  if ( -1 == result ) {
    root->left = insert( tree, node, root->left );
  } else if ( 1 == result ) {
    root->right = insert( tree, node, root->right );
  } else {
    root->right = insert( tree, node, root->right );
  }

  // return
  return balance( root );
}

/**
 * @brief Insert node into existing tree
 *
 * @param tree
 * @param node
 */
void avl_insert_by_node( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  // ensure existing tree
  assert( NULL != tree && NULL != node );

  // insert and rebalance
  tree->root = insert( tree, node, tree->root );
}
