
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include "kernel/kernel/panic.h"
#include "lib/avl/avl.h"

/**
 * @brief Simple helper for retrieving max between two 32 bit integers
 *
 * @param a
 * @param b
 * @return int32_t
 */
static inline int32_t max( int32_t a, int32_t b ) {
  return a > b ? a : b;
}

/**
 * @brief Simple helper for retrieving min between two 32 bit integers
 *
 * @param a
 * @param b
 * @return int32_t
 */
static inline int32_t min( int32_t a, int32_t b ) {
  return a > b ? b : a;
}

/**
 * @brief Calculate node height
 *
 * @param node
 * @return int32_t
 */
static int32_t height( avl_node_ptr_t node ) {
  // handle null value
  if ( NULL == node ) {
    return 0;
  }

  // calculate height of left and right
  int32_t left = height( node->left );
  int32_t right = height( node->right );

  // return bigger height
  return left > right
    ? left++
    : right++;
}

/**
 * @brief Helper to get the balance factor for a node
 *
 * @param node
 * @return int32_t
 */
static int32_t balance_factor( avl_node_ptr_t node ) {
  // handle invalid node ( balanced )
  if ( NULL == node ) {
    return 0;
  }

  // return tree balance by using height
  return height( node->right ) - height( node->left );
}

static avl_node_ptr_t rotate_right( avl_node_ptr_t node ) {
  // cache left node within temporary
  avl_node_ptr_t tmp = node->left;

  // set right of tmp as nodes left
  node->left = tmp->right;
  tmp->right = node;

  // return new root node after rotation
  return tmp;
}

static avl_node_ptr_t rotate_left( avl_node_ptr_t node ) {
  // cache left node within temporary
  avl_node_ptr_t tmp = node->right;

  // set right of tmp as nodes left
  node->right = tmp->left;
  tmp->left = node;

  // return new root node after rotation
  return tmp;
}

/**
 * @brief Method to balance node with return of new root node
 *
 * @param node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t balance( avl_node_ptr_t node ) {
  // left / right left rotation
  if ( 2 == balance_factor( node ) ) {
    // right rotation?
    if ( 0 > balance_factor( node->right ) ) {
      node->right = rotate_right( node->right );
    }

    // left rotation
    return rotate_left( node );
  }

  // left / left right rotation
  if ( -2 == balance_factor( node ) ) {
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
  if ( -1 == tree->compare( root, node, tree->param ) ) {
    if ( NULL != root->left ) {
      root->left = insert( tree, node, root->left );
    } else {
      root->left = node;
    }
  } else if ( 1 == tree->compare( root, node, tree->param ) ) {
    if ( NULL != root->right ) {
      root->right = insert( tree, node, root->right );
    } else {
      root->right = node;
    }
  }

  return balance( root );
}

/**
 * @brief Creates a new avl tree
 *
 * @param func
 * @param param
 * @return avl_tree_ptr_t
 */
avl_tree_ptr_t avl_create( avl_compare_func_t func, void* param ) {
  ( void )func;
  ( void )param;

  return NULL;
}

/**
 * @brief Insert node into existing tree
 *
 * @param tree
 * @param node
 * @return bool
 */
void avl_insert( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  // ensure existing tree
  ASSERT( NULL != tree && NULL != node );

  // insert and rebalance
  tree->root = insert( tree, node, tree->root );
}
