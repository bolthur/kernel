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

#include "avl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief buffer helper for output
 */
static uint8_t level_buffer[ 4096 ];

/**
 * @brief depth index
 */
static int32_t level_index;

/**
 * @brief Internal function for inserting a node
 *
 * @param tree
 * @param node
 * @param root
 * @return avl_node_t*
 */
static avl_node_t* insert(
  avl_tree_t* tree,
  avl_node_t* node,
  avl_node_t* root
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
 * @brief Helper to find node within tree
 *
 * @param data data to lookup for
 * @param root root node
 * @param tree
 * @return avl_node_t*
 */
static avl_node_t* find_by_data(
  void* data,
  avl_node_t* root,
  const avl_tree_t* tree
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
 * @return avl_node_t*
 */
static avl_node_t* find_parent_by_data(
  void* data,
  avl_node_t* root,
  const avl_tree_t* tree
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
 * @brief Calculate node height
 *
 * @param node
 * @return int32_t
 */
static int32_t height( avl_node_t* node ) {
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
static int32_t balance_factor( avl_node_t* node ) {
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
 * @return avl_node_t*
 */
static avl_node_t* rotate_right( avl_node_t* node ) {
  // cache left node within temporary
  avl_node_t* left = node->left;

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
 * @return avl_node_t*
 */
static avl_node_t* rotate_left( avl_node_t* node ) {
  // cache left node within temporary
  avl_node_t* right = node->right;

  // set right of tmp as nodes left
  node->right = right->left;
  right->left = node;

  // return new root node after rotation
  return right;
}

/**
 * @brief Helper to get previous node
 *
 * @param current
 * @param root
 * @param tree
 * @return avl_node_t*
 */
static avl_node_t* find_previous_node(
  avl_node_t* current,
  avl_node_t* root,
  avl_tree_t* tree
) {
  if ( ! root ) {
    return NULL;
  }

  // handle root node
  if ( root == current ) {
    // handle left node existing ( max )
    if ( root->left ) {
      return avl_get_max( root->left );
    }
    // handle right node existing ( min )
    if ( root->right ) {
      return avl_get_min( root->right );
    }
    // nothing existing
    return NULL;
  }

  int32_t result = tree->compare( root, current );
  if ( -1 == result ) {
    return find_previous_node( current, root->left, tree );
  } else {
    return find_previous_node( current, root->right, tree );
  }
}

/**
 * @brief Get the max object
 *
 * @param node current node
 * @param result result node
 * @return avl_node_t*
 */
static avl_node_t* get_max(
  avl_node_t* node,
  avl_node_t* result
) {
  // break if we reached the maximum
  if ( ! node ) {
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
 * @return avl_node_t*
 */
static avl_node_t* get_min(
  avl_node_t* node,
  avl_node_t* result
) {
  // break if we reached the maximum
  if ( ! node ) {
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
 * @return avl_node_t* new root
 */
static avl_node_t* remove_by_node(
  avl_tree_t* tree,
  avl_node_t* node,
  avl_node_t* root
) {
  // recursive breakpoint
  if ( ! root ) {
    return NULL;
  }

  // get result
  int32_t result = tree->compare( node, root );

  // equal? => check for root is node and continue on subtrees if not
  if ( 0 == result ) {
    // equal but not the found one, check both subtrees
    if ( node != root ) {
      // continue on left subtree if existing
      if ( root->left ) {
        root->left = remove_by_node( tree, node, root->left );
      }

      // continue on right subtree if existing
      if ( root->right ) {
        root->right = remove_by_node( tree, node, root->right );
      }
    } else {
      avl_node_t* tmp;

      // no child or one child, just return child or NULL
      if ( ! root->left || ! root->right ) {
        // get temporary
        tmp = root->left
          ? root->left
          : root->right;

        // no child
        if ( ! tmp ) {
          root = NULL;
        // one child
        } else {
          // overwrite root
          root = tmp;
        }
      } else {
        // get the smallest successor of right node
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
  if ( ! root ) {
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
 * @return avl_node_t*
 */
static avl_node_t* remove_by_data(
  void* data,
  avl_node_t* root
) {
  // recursive breakpoint
  if ( ! root ) {
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
    avl_node_t* tmp;

    // no child or one child, just return child or NULL
    if ( ! root->left || ! root->right ) {
      // get temporary
      tmp = root->left
        ? root->left
        : root->right;

      // no child
      if ( ! tmp ) {
        root = NULL;
      // one child
      } else {
        // overwrite root
        root = tmp;
      }
    } else {
      // get the smallest successor of right node
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
  if ( ! root ) {
    return root;
  }

  // return rebalanced partial root
  return balance( root );
}

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
 * @brief Recursive print of tree
 *
 * @param node
 */
static void print_recursive( const avl_node_t* node ) {
  if ( ! node ) {
    return;
  }

  if ( level_index ) {
    printf( "%s `--%p\r\n", level_buffer, node->data );
  } else {
    printf( "%p\r\n", node->data );
  }

  if ( node->left ) {
    push_output_level( '|' );
    print_recursive( node->left );
    pop_output_level();
  }

  if ( node->right ) {
    push_output_level( '|' );
    print_recursive( node->right );
    pop_output_level();
  }
}

/**
 * @brief Default lookup if not passed during creation
 *
 * @param a
 * @param b
 * @return int32_t
 */
int32_t avl_default_lookup( const avl_node_t* a, const void* b ) {
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
void avl_default_cleanup( __unused avl_node_t* a ) {}

/**
 * @brief Helper to create new tree
 *
 * @param compare compare function to be used within tree
 * @param lookup
 * @param cleanup
 * @return avl_tree_t* pointer to new tree
 */
avl_tree_t* avl_create_tree(
  avl_compare_func_t compare,
  avl_lookup_func_t lookup,
  avl_cleanup_func_t cleanup
) {
  // reserve space for new tree structure
  avl_tree_t* new_tree = ( avl_tree_t* )malloc( sizeof( avl_tree_t ) );
  // check
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
avl_node_t* avl_create_node( void* data ) {
  // reserve space for new node
  avl_node_t* node = ( avl_node_t* )malloc( sizeof( avl_node_t ) );
  // check
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

/**
 * @brief Helper to destroy created tree
 *
 * @param tree
 */
void avl_destroy_tree( avl_tree_t* tree ) {
  // check parameter
  if ( ! tree ) {
    return;
  }

  // loop as long a root node is existing
  while ( tree->root ) {
    // cache root node
    avl_node_t* node = tree->root;
    // remove node from tree
    avl_remove_by_node( tree, node );
    // cleanup
    tree->cleanup( node );
  }
  // finally, free tree itself
  free( tree );
}

/**
 * @brief Insert node into existing tree
 *
 * @param tree
 * @param node
 * @return true
 * @return false
 */
bool avl_insert_by_node( avl_tree_t* tree, avl_node_t* node ) {
  // check parameter
  if ( ! tree || ! node ) {
    return false;
  }
  // insert and balance
  tree->root = insert( tree, node, tree->root );
  return true;
}

/**
 * @brief Find an avl node within tree
 *
 * @param tree tree to search
 * @param data data to lookup
 * @return avl_node_t* found node or NULL
 */
avl_node_t* avl_find_by_data( const avl_tree_t* tree, void* data ) {
  return find_by_data( data, tree->root, tree );
}

/**
 * @brief Find parent
 *
 * @param tree tree to work on
 * @param data data to lookup
 */
avl_node_t* avl_find_parent_by_data( const avl_tree_t* tree, void* data ) {
  return find_parent_by_data( data, tree->root, tree );
}

/**
 * @brief Method to balance node with return of new root node
 *
 * @param node
 * @return avl_node_t*
 */
avl_node_t* balance( avl_node_t* node ) {
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
 * @brief Get first node
 *
 * @param tree avl tree
 */
avl_node_t* avl_iterate_first( avl_tree_t* tree ) {
  if ( ! tree ) {
    return NULL;
  }
  return avl_get_min( tree->root );
}

/**
 * @brief Get last node
 *
 * @param tree avl tree
 */
avl_node_t* avl_iterate_last( avl_tree_t* tree ) {
  if ( ! tree ) {
    return NULL;
  }
  return avl_get_max( tree->root );
}

/**
 * @brief Get next node
 *
 * @param tree avl tree
 * @param node node
 */
avl_node_t* avl_iterate_next( avl_tree_t* tree, avl_node_t* node ) {
  if ( ! tree || ! node || ! tree->root ) {
    return NULL;
  }

  // handle right element is existing
  if ( node->right ) {
    // return smallest node of right subtree
    return avl_get_min( node->right );
  }

  avl_node_t* next = NULL;
  avl_node_t* root = tree->root;
  // search from root
  while ( root ) {
    // compare to determine result
    int32_t result = tree->compare( root, node );

    // exit point
    if ( 0 == result ) {
      break;
    }

    if ( -1 == result ) {
      next = root;
      root = root->left;
    } else {
      root = root->right;
    }
  }

  return next;
}

/**
 * @brief Get previous node
 *
 * @param tree avl tree
 * @param node node
 */
avl_node_t* avl_iterate_previous( avl_tree_t* tree, avl_node_t* node ) {
  if ( ! tree || ! node ) {
    return NULL;
  }
  return find_previous_node( node, tree->root, tree );
}

/**
 * @brief Get max node of tree
 *
 * @param root root to get max node
 * @return avl_node_t* found node or null if empty
 */
avl_node_t* avl_get_max( avl_node_t* root ) {
  return get_max( root, NULL );
}

/**
 * @brief Get min node of tree
 *
 * @param root node to get min value
 * @return avl_node_t* found node or null if empty
 */
avl_node_t* avl_get_min( avl_node_t* root ) {
  return get_min( root, NULL );
}

/**
 * @brief method to prepare some node
 *
 * @param node node to prepare
 * @param data initial node data
 */
void avl_prepare_node( avl_node_t* node, void* data ) {
  node->left = NULL;
  node->right = NULL;
  node->data = data;
}

/**
 * @brief Debug output avl tree
 *
 * @param tree
 */
void avl_print( const avl_tree_t* tree ) {
  if ( ! tree->root ) {
    printf( "( empty tree )\r\n" );
    return;
  }
  print_recursive( tree->root );
}

/**
 * @brief Remove an avl node from tree
 *
 * @param tree tree to search in
 * @param data data of node to find
 */
void avl_remove_by_data( avl_tree_t* tree, void* data ) {
  tree->root = remove_by_data( data, tree->root );
}

/**
 * @brief Removes an avl tree by node
 *
 * @param tree tree to work on
 * @param node node to remove
 */
void avl_remove_by_node( avl_tree_t* tree, avl_node_t* node ) {
  tree->root = remove_by_node( tree, node, tree->root );
}
