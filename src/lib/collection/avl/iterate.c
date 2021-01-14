
/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <string.h>
#include <collection/avl.h>

/**
 * @brief Helper to get previous node
 *
 * @param current
 * @param root
 * @param tree
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t find_previous_node(
  avl_node_ptr_t current,
  avl_node_ptr_t root,
  avl_tree_ptr_t tree
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
 * @brief Get first node
 *
 * @param tree avl tree
 */
avl_node_ptr_t avl_iterate_first( avl_tree_ptr_t tree ) {
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
avl_node_ptr_t avl_iterate_last( avl_tree_ptr_t tree ) {
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
avl_node_ptr_t avl_iterate_next( avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  if ( ! tree || ! node || ! tree->root ) {
    return NULL;
  }

  // handle right element is existing
  if ( node->right ) {
    // step right
    node = node->right;
    // return smallest node of right sub tree
    return avl_get_min( node->right );
  }

  avl_node_ptr_t next = NULL;
  avl_node_ptr_t root = tree->root;
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
avl_node_ptr_t avl_iterate_previous( avl_tree_ptr_t tree, avl_node_ptr_t node ) {
  if ( ! tree || ! node ) {
    return NULL;
  }
  return find_previous_node( node, tree->root, tree );
}
