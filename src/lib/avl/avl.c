
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

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avl.h>

/**
 * @brief buffer helper for output
 */
static uint8_t level_buffer[ 4096 ];

/**
 * @brief depth index
 */
static int32_t level_index;

/**
 * @brief Push something to depth buffer
 *
 * @param c
 */
static void push_output_level( uint8_t c ) {
  level_buffer[ level_index++ ] = ' ';
  level_buffer[ level_index++ ] = c;
  level_buffer[ level_index++ ] = ' ';
  level_buffer[ level_index++ ] = ' ';
  level_buffer[ level_index ] = 0;
}

/**
 * @brief Pop something from depth
 */
static void pop_output_level( void ) {
  level_index -= 4;
  level_buffer[ level_index ] = 0;
}

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
  if ( NULL == node ) {
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
 * @brief Recursive print of tree
 *
 * @param node
 */
static void print_recursive( const avl_node_ptr_t node ) {
  if ( NULL == node ) {
    return;
  }
  printf( "%p\r\n", node->data );

  if ( node->left ) {
    printf( "%s `--", level_buffer );
    push_output_level( '|' );
    print_recursive( node->left );
    pop_output_level();
  }

  if ( node->right ) {
    printf( "%s `--", level_buffer );
    push_output_level( '|' );
    print_recursive( node->right );
    pop_output_level();
  }
}

/**
 * @brief Method to balance node with return of new root node
 *
 * @param node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t balance( avl_node_ptr_t node ) {
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
 * @brief Get the max object
 *
 * @param node current node
 * @param result result node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t get_max(
  avl_node_ptr_t node,
  avl_node_ptr_t result
) {
  // break if we reached the maximum
  if ( NULL == node ) {
    return result;
  }

  // just step to right for max entry
  return get_max( node->right, node );
}

/**
 * @brief Get the min object
 *
 * @param node current node
 * @param result result node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t get_min(
  avl_node_ptr_t node,
  avl_node_ptr_t result
) {
  // break if we reached the maximum
  if ( NULL == node ) {
    return result;
  }

  // just step to right for max entry
  return get_min( node->left, node );
}

/**
 * @brief Helper to remove by node
 *
 * @param tree tree to work on
 * @param node node to remove
 * @param root current root
 * @return avl_node_ptr_t new root
 */
static avl_node_ptr_t remove_by_node(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t root
) {
  // recursive breakpoint
  if ( NULL == root ) {
    return NULL;
  }

  // get result
  int32_t result = tree->compare( node, root );

  // equal? => check for root is node and continue on subtrees if not
  if ( 0 == result ) {
    // equal but not the found one, check both sub trees
    if ( node != root ) {
      // continue on left subtree if existing
      if ( NULL != root->left ) {
        root->left = remove_by_node( tree, node, root->left );
      }

      // continue on right subtree if existing
      if ( NULL != root->right ) {
        root->right = remove_by_node( tree, node, root->right );
      }
    } else {
      avl_node_ptr_t tmp;

      // no child or one child, just return child or NULL
      if ( NULL == root->left || NULL == root->right ) {
        // get temporary
        tmp = NULL != root->left
          ? root->left
          : root->right;

        // no child
        if ( NULL == tmp ) {
          root = NULL;
        // one child
        } else {
          // overwrite root
          root = tmp;
        }
      } else {
        // get smallest successor of right node
        tmp = avl_get_min( root->right );

        // remove tmp from right
        root->right = remove_by_node( tree, tmp, root->right );

        // replace current one with tmp
        tmp->left = root->left;
        tmp->right = root->right;

        // overwrite root
        root = tmp;
      }
    }
  // continue on left
  } else if ( 1 == result ) {
    root->left = remove_by_node( tree, node, root->left );
  // continue on right
  } else if ( -1 == result ) {
    root->right = remove_by_node( tree, node, root->right );
  }

  // break if tree is empty
  if ( NULL == root ) {
    return root;
  }

  // return rebalanced partial root
  return balance( root );
}

/**
 * @brief Recursive remove by value
 *
 * @param data data to remove
 * @param root root node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t remove_by_data(
  void* data,
  avl_node_ptr_t root
) {
  // recursive breakpoint
  if ( NULL == root ) {
    return NULL;
  }

  // continue left
  if ( root->data > data ) {
    root->left = remove_by_data( data, root->left );
  // continue right
  } else if ( data > root->data ) {
    root->right = remove_by_data( data, root->right );
  // found node
  } else {
    avl_node_ptr_t tmp;

    // no child or one child, just return child or NULL
    if ( NULL == root->left || NULL == root->right ) {
      // get temporary
      tmp = NULL != root->left
        ? root->left
        : root->right;

      // no child
      if ( NULL == tmp ) {
        root = NULL;
      // one child
      } else {
        // overwrite root
        root = tmp;
      }
    } else {
      // get smallest successor of right node
      tmp = avl_get_min( root->right );

      // remove tmp from right
      root->right = remove_by_data( tmp->data, root->right );

      // replace current one with tmp
      tmp->left = root->left;
      tmp->right = root->right;

      // overwrite root
      root = tmp;
    }
  }

  // break if tree is empty
  if ( NULL == root ) {
    return root;
  }

  // return rebalanced partial root
  return balance( root );
}

/**
 * @brief Creates a new avl tree
 *
 * @param func
 * @return avl_tree_ptr_t
 */
avl_tree_ptr_t avl_create( avl_compare_func_t func ) {
  // create tree
  avl_tree_ptr_t new_tree = ( avl_tree_ptr_t )calloc( 1, sizeof( avl_tree_t ) );

  // initialize attributes
  new_tree->compare = func;
  new_tree->root = NULL;

  // finally return
  return new_tree;
}

/**
 * @brief Insert node into existing tree
 *
 * @param tree
 * @param node
 * @return bool
 */
void avl_insert_by_node( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  // ensure existing tree
  assert( NULL != tree && NULL != node );

  // insert and rebalance
  tree->root = insert( tree, node, tree->root );
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

/**
 * @brief Remove an avl node from tree
 *
 * @param tree tree to search in
 * @param data data of node to find
 */
void avl_remove_by_data( const avl_tree_ptr_t tree, void* data ) {
  tree->root = remove_by_data( data, tree->root );
}

/**
 * @brief Removes an avl tree by node
 *
 * @param tree tree to work on
 * @param node node to remove
 */
void avl_remove_by_node( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  tree->root = remove_by_node( tree, node, tree->root );
}

/**
 * @brief Get max node of tree
 *
 * @param root node to get min value
 * @return avl_node_ptr_t found node or null if empty
 */
avl_node_ptr_t avl_get_max( const avl_node_ptr_t root ) {
  return get_max( root, NULL );
}

/**
 * @brief Get min node of tree
 *
 * @param root node to get min value
 * @return avl_node_ptr_t found node or null if empty
 */
avl_node_ptr_t avl_get_min( const avl_node_ptr_t root ) {
  return get_min( root, NULL );
}

/**
 * @brief Debug output avl tree
 *
 * @param tree
 */
void avl_print( const avl_tree_ptr_t tree ) {
  print_recursive( tree->root );
}

/**
 * @brief method to prepare some node
 *
 * @param node node to prepare
 * @param data initial node data
 */
void avl_prepare_node( avl_node_ptr_t node, void* data ) {
  node->left = NULL;
  node->right = NULL;
  node->data = data;
}
