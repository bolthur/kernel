
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

#include <assert.h>
#include <avl/avl.h>

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
  // handle empty root
  if ( NULL == root ) {
    return node;
  }

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
 * @brief Helper to find node within tree
 *
 * @param tree tree to search
 * @param node node to find
 * @param root root node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t root
) {
  // end point
  if ( root == NULL ) {
    return NULL;
  }

  // determine result
  int32_t result = tree->compare( node, root, tree->param);

  // continue left
  if ( -1 == result ) {
    return find( tree, node, root->left );
  // continue right
  } else if ( 1 == result ) {
    return find( tree, node, root->right );
  }

  // generic else case: found node is the wanted one
  return root;
}

/**
 * @brief Get the max object
 *
 * @param tree avl tree structure
 * @param node current node
 * @param result result node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t get_max(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t result
) {
  // break if we reached the maximum
  if ( NULL == node ) {
    return result;
  }

  // just step to right for max entry
  return get_max( tree, node->right, node );
}

/**
 * @brief Get the min object
 *
 * @param tree avl tree structure
 * @param node current node
 * @param result result node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t get_min(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t result
) {
  // break if we reached the maximum
  if ( NULL == node ) {
    return result;
  }

  // just step to right for max entry
  return get_min( tree, node->left, node );
}

/**
 * @brief Helper for deletion of left wing recursively
 *
 * @param node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t remove_left_recursive( avl_node_ptr_t node ) {
  // return right, when left is NULL
  if ( NULL == node->left ) {
    return node->right;
  }

  // remove left recursively and change left sibling
  node->left = remove_left_recursive( node->left );

  // return balanced node
  return balance( node );
}

/**
 * @brief Recursive remove by node
 *
 * @param tree tree to search at
 * @param node node to get parent of
 * @param root root node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t remove(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t root
) {
  // compare
  int32_t result = tree->compare( node, root, tree->param );

  // current node is the one to delete
  if ( 0 == result ) {
    // no child or one child, just return child or NULL
    if ( NULL == root->left || NULL == root->right ) {
      return NULL != root->left
        ? root->left
        : root->right;
    }
    // get minimum of right child for new root
    avl_node_ptr_t left = get_min( tree, root, root->right );
    left->right = remove_left_recursive( root->right );
    left->left = root->left;
    // return rebalanced node
    return balance( left );
  // continue left
  } else if ( -1 == result ) {
    root->left = remove( tree, node, root->left );
  // continue right
  } else if ( 1 == result ) {
    root->right = remove( tree, node, root->right );
  }

  // return rebalanced partial root
  return balance( root );
}

/**
 * @brief Recursive find parent by node
 *
 * @param tree tree to search at
 * @param node node to get parent of
 * @param root root node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find_parent(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node,
  avl_node_ptr_t root
) {
  // end point
  if ( root == NULL ) {
    return NULL;
  }

  // determine result
  int32_t result = tree->compare( node, root, tree->param);

  if (
    (
      NULL != root->right
      && 0 == tree->compare( node, root->left, tree->param )
    ) || (
      NULL != root->right
      && 0 == tree->compare( node, root->right, tree->param )
    )
  ) {
    return root;
  }

  // continue left
  if ( -1 == result ) {
    return find( tree, node, root->left );
  // continue right
  } else if ( 1 == result ) {
    return find( tree, node, root->right );
  }

  // generic else case: found node is the wanted one without parent
  return NULL;
}

/**
 * @brief Creates a new avl tree
 *
 * @param func
 * @param param
 * @return avl_tree_ptr_t
 *
 * @todo add logic if necessary, else remove
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
  assert( NULL != tree && NULL != node );

  // insert and rebalance
  tree->root = insert( tree, node, tree->root );
}

/**
 * @brief Find an avl node within treee
 *
 * @param tree tree to search
 * @param node node to find
 * @return avl_node_ptr_t found node or NULL
 */
avl_node_ptr_t avl_find( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  return find( tree, node, tree->root );
}

/**
 * @brief Remove an avl node from tree
 *
 * @param tree tree to search in
 * @param node node to find
 */
void avl_remove( const avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  tree->root = remove( tree, node, tree->root );
}

/**
 * @brief Find parent of node within tree
 *
 * @param tree tree to search at
 * @param node node to get parent of
 * @return avl_node_ptr_t found parent or NULL
 */
avl_node_ptr_t avl_find_parent(
  const avl_tree_ptr_t tree,
  avl_node_ptr_t node
) {
  return find_parent( tree, node, tree->root );
}

/**
 * @brief Get max node of tree
 *
 * @param tree tree to look up
 * @return avl_node_ptr_t found node or null if empty
 */
avl_node_ptr_t avl_get_max( const avl_tree_ptr_t tree ) {
  return get_max( tree, tree->root, NULL );
}

/**
 * @brief Get min node of tree
 *
 * @param tree tree to look up
 * @return avl_node_ptr_t found node or null if empty
 */
avl_node_ptr_t avl_get_min( const avl_tree_ptr_t tree ) {
  return get_min( tree, tree->root, NULL );
}
