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
 * @brief Calculate node height
 *
 * @param node
 * @return int32_t
 */
static int32_t height( avl_node_ptr_t node ) {
  // handle null value
  if ( ! node ) {
    return 0;
  }

  // calculate height of left and right
  int32_t left = height( node->left );
  int32_t right = height( node->right );

  // return bigger height
  return left > right
    ? ++left
    : ++right;
}

/**
 * @brief Helper to get the balance factor for a node
 *
 * @param node
 * @return int32_t
 */
static int32_t balance_factor( avl_node_ptr_t node ) {
  // handle invalid node ( balanced )
  if ( ! node ) {
    return 0;
  }

  // return tree balance by using height
  return height( node->right ) - height( node->left );
}

/**
 * @brief Right rotation
 *
 * @param node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t rotate_right( avl_node_ptr_t node ) {
  // cache left node within temporary
  avl_node_ptr_t left = node->left;

  // set right of tmp as nodes left
  node->left = left->right;
  left->right = node;

  // return new root node after rotation
  return left;
}

/**
 * @brief Left rotation
 *
 * @param node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t rotate_left( avl_node_ptr_t node ) {
  // cache left node within temporary
  avl_node_ptr_t right = node->right;

  // set right of tmp as nodes left
  node->right = right->left;
  right->left = node;

  // return new root node after rotation
  return right;
}

/**
 * @brief Method to balance node with return of new root node
 *
 * @param node
 * @return avl_node_ptr_t
 */
avl_node_ptr_t balance( avl_node_ptr_t node ) {
  // get balance factor
  int32_t balance = balance_factor( node );

  // left / right left rotation
  if ( 2 == balance ) {
    // right rotation?
    if ( 0 > balance_factor( node->right ) ) {
      node->right = rotate_right( node->right );
    }
    // left rotation
    return rotate_left( node );
  }

  // left / left right rotation
  if ( -2 == balance ) {
    // left rotation
    if ( 0 < balance_factor( node->left ) ) {
      node->left = rotate_left( node->left );
    }
    // right rotation
    return rotate_right( node );
  }

  // no further balance necessary
  return node;
}
