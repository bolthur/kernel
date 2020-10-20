
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

/**
 * @brief Helper to find node within tree
 *
 * @param data data to lookup for
 * @param root root node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find_by_data(
  void* data,
  avl_node_ptr_t root
) {
  // end point
  if ( root == NULL ) {
    return NULL;
  }

  // continue left
  if ( root->data > data ) {
    return find_by_data( data, root->left );
  // continue right
  } else if ( data > root->data ) {
    return find_by_data( data, root->right );
  }

  // generic else case: found node is the wanted one
  return root;
}

/**
 * @brief Helper to find parent node within tree
 *
 * @param data data to lookup for
 * @param root root node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find_parent_by_data(
  void* data,
  avl_node_ptr_t root
) {
  // end point
  if ( root == NULL ) {
    return NULL;
  }

  // matching node left?
  if (
      NULL != root->left
      && root->left->data == data
  ) {
    return root;
  }

  // matching node right?
  if (
    NULL != root->right
    && root->right->data == data
  ) {
    return root;
  }

  // continue left
  if ( root->data > data ) {
    return find_parent_by_data( data, root->left );
  // continue right
  } else if ( data > root->data ) {
    return find_parent_by_data( data, root->right );
  }

  // generic else case: found node is the wanted one
  return NULL;
}

/**
 * @brief Find an avl node within treee
 *
 * @param tree tree to search
 * @param data data to lookup
 * @return avl_node_ptr_t found node or NULL
 */
avl_node_ptr_t avl_find_by_data( const avl_tree_ptr_t tree, void* data ) {
  return find_by_data( data, tree->root );
}

/**
 * @brief Find parent
 *
 * @param tree tree to work on
 * @param data data to lookup
 */
avl_node_ptr_t avl_find_parent_by_data( const avl_tree_ptr_t tree, void* data ) {
  return find_parent_by_data( data, tree->root );
}
