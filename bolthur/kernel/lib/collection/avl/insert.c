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

#include <collection/avl.h>

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
  if ( ! root ) {
    return node;
  }

  int32_t result = tree->compare( root, node );

  if ( -1 == result ) {
    root->left = insert( tree, node, root->left );
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
 * @return true
 * @return false
 */
bool avl_insert_by_node( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  // check parameter
  if ( ! tree || ! node ) {
    return false;
  }
  // insert and balance
  tree->root = insert( tree, node, tree->root );
  return true;
}
