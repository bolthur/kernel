
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avl.h>

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
