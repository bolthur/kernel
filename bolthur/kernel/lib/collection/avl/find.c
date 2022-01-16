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
 * @brief Helper to find node within tree
 *
 * @param data data to lookup for
 * @param root root node
 * @param tree
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find_by_data(
  void* data,
  avl_node_ptr_t root,
  avl_tree_ptr_t tree
) {
  // end point
  if ( ! root || ! tree ) {
    return NULL;
  }

  // check result
  int32_t result = tree->lookup( root, data );
  // handle match
  if ( 0 == result ) {
    return root;
  }

  // continue left
  if ( -1 == result ) {
    return find_by_data( data, root->left, tree );
  // continue right
  } else {
    return find_by_data( data, root->right, tree );
  }
}

/**
 * @brief Helper to find parent node within tree
 *
 * @param data data to lookup for
 * @param root root node
 * @param tree
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find_parent_by_data(
  void* data,
  avl_node_ptr_t root,
  avl_tree_ptr_t tree
) {
  // end point
  if ( ! root || ! tree ) {
    return NULL;
  }

  // matching node left?
  if (
    root->left
    && 0 == tree->lookup( root->left, data )
  ) {
    return root;
  }

  // matching node right?
  if (
    root->right
    && 0 == tree->lookup( root->right, data )
  ) {
    return root;
  }

  int32_t result = tree->lookup( root, data );

  // continue left
  if ( -1 == result ) {
    return find_parent_by_data( data, root->left, tree );
  // continue right
  } else if ( 1 == result ) {
    return find_parent_by_data( data, root->right, tree );
  }

  // generic else case: found node is the wanted one
  return NULL;
}

/**
 * @brief Find an avl node within tree
 *
 * @param tree tree to search
 * @param data data to lookup
 * @return avl_node_ptr_t found node or NULL
 */
avl_node_ptr_t avl_find_by_data( const avl_tree_ptr_t tree, void* data ) {
  return find_by_data( data, tree->root, tree );
}

/**
 * @brief Find parent
 *
 * @param tree tree to work on
 * @param data data to lookup
 */
avl_node_ptr_t avl_find_parent_by_data( const avl_tree_ptr_t tree, void* data ) {
  return find_parent_by_data( data, tree->root, tree );
}
