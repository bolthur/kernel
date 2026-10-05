/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include <inttypes.h>

#define AVL_INSERT_REMOVE_MAX 64

/**
 * @brief buffer helper for output
 */
static uint8_t level_buffer[ 4096 ];

/**
 * @brief depth index
 */
static int32_t level_index;

/**
 * @fn int32_t height(avl_node_t*)
 * @brief Get nodes height
 * @param node
 * @return int32_t
 */
__attribute__((always_inline)) static inline int32_t height_get( const avl_node_t* node ) {
  return node ? node->height : 0;
}

/**
 * @fn int32_t max(int32_t, int32_t)
 * @brief Small max helper
 * @param left
 * @param right
 * @return
 */
__attribute__((always_inline)) static inline int32_t max( const int32_t left, const int32_t right ) {
  return left > right ? left : right;
}

/**
 * @fn void height_update(avl_node_t*)
 * @brief Helper to update height
 * @param node
 */
__attribute__((always_inline)) static inline void height_update( avl_node_t* node ) {
  if ( node ) {
    node->height = 1 + max( height_get( node->left ), height_get( node->right ) );
  }
}

/**
 * @fn avl_node_t insert*(avl_tree_t*, avl_node_t*, avl_node_t*)
 * @brief Internal function for inserting a node
 * @param tree
 * @param node
 * @param root
 * @return
 */
static avl_node_t* insert( const avl_tree_t* tree, avl_node_t* node, avl_node_t* root ) {
  // handle empty root
  if ( ! root ) {
    return node;
  }
  // space for balancing path
  avl_node_t* path[ AVL_INSERT_REMOVE_MAX ];
  uint32_t path_top = 0;
  // start with root
  auto current = root;
  // iterate current
  while ( current ) {
    // cache current
    path[ path_top++ ] = current;
    // check where to traverse
    const int32_t result = tree->compare( current, node );
    // handle left hand
    if ( -1 == result ) {
      if ( ! current->left ) {
        current->left = node;
        break;
      }
      current = current->left;
    // handle right hand
    } else if ( 1 == result ) {
      if ( ! current->right ) {
        current->right = node;
        break;
      }
      current = current->right;
    // handle equal
    } else {
      return root;
    }
  }
  // balance from top to bottom
  while ( path_top > 0 ) {
    // get last sub node
    avl_node_t* sub_node = path[ --path_top ];
    // update height
    height_update( sub_node );
    // balance it
    avl_node_t* balanced_node = balance( sub_node );
    // handle balanced node is not sub node
    if ( balanced_node != sub_node ) {
      // handle remaining nodes
      if ( path_top > 0 ) {
        // get parent
        avl_node_t* parent = path[ path_top - 1 ];
        // change left / right to balanced node
        if ( parent->left == sub_node ) {
          parent->left = balanced_node;
        } else {
          parent->right = balanced_node;
        }
      // change root node
      } else {
        root = balanced_node;
      }
      // skip rest
      break;
    }
  }
  // return root node
  return root;
}

/**
 * @fn int32_t balance_factor(avl_node_t*)
 * @brief Helper to get the balance factor for a node
 * @param node
 * @return int32_t
 */
__attribute__((always_inline)) static inline int32_t balance_factor( avl_node_t* node ) {
  return ! node ? 0 : height_get( node->right ) - height_get( node->left );
}

/**
 * @fn avl_node_t rotate_right*(avl_node_t*)
 * @brief Right rotation
 * @param node
 * @return avl_node_t*
 */
static avl_node_t* rotate_right( avl_node_t* node ) {
  // cache left node within temporary
  avl_node_t* left = node->left;
  // set right of tmp as nodes left
  node->left = left->right;
  left->right = node;
  // update heights
  height_update( node );
  height_update( left );
  // return new root node after rotation
  return left;
}

/**
 * @fn avl_node_t rotate_left*(avl_node_t*)
 * @brief Left rotation
 * @param node
 * @return avl_node_t*
 */
static avl_node_t* rotate_left( avl_node_t* node ) {
  // cache left node within temporary
  avl_node_t* right = node->right;
  // set right of tmp as nodes left
  node->right = right->left;
  right->left = node;
  // update heights
  height_update( node );
  height_update( right );
  // return new root node after rotation
  return right;
}

/**
 * @fn avl_node_t remove_by_node*(const avl_tree_t*, const avl_node_t*, avl_node_t*)
 * @brief Helper to remove by node
 * @param tree tree to work on
 * @param node node to remove
 * @param root current root
 * @return avl_node_t* new root
 */
static avl_node_t* remove_by_node(
  const avl_tree_t* tree,
  const avl_node_t* node,
  avl_node_t* root
) {
  // recursive breakpoint
  if ( ! root ) {
    return nullptr;
  }
  avl_node_t* path[ AVL_INSERT_REMOVE_MAX ];
  uint32_t path_top = 0;
  avl_node_t* current = root;
  // find node to remove
  while ( current && current != node ) {
    path[ path_top++ ] = current;
    const int32_t result = tree->compare( node, current );
    if ( 1 == result ) {
      current = current->left;
    } else {
      current = current->right;
    }
  }
  // cache node to delete
  avl_node_t* node_to_delete = current;
  // handle not found
  if ( ! node_to_delete ) {
    return root;
  }
  // handle only one child
  if ( ! node_to_delete->left || ! node_to_delete->right ) {
    // get child
    avl_node_t* child = node_to_delete->left
      ? node_to_delete->left : node_to_delete->right;
    // handle node to delete is root
    if ( path_top == 0 ) {
      root = child;
    } else {
      avl_node_t* parent = path[ path_top - 1 ];
      if ( parent->left == node_to_delete ) {
        parent->left = child;
      } else {
        parent->right = child;
      }
    }
  // node has left and right child
  } else {
    // get successor
    uint32_t successor_top = 0;
    avl_node_t* successor_stack[ AVL_INSERT_REMOVE_MAX ];
    avl_node_t* successor = node_to_delete->right;
    while ( successor->left ) {
      successor_stack[ successor_top++ ] = successor;
      successor = successor->left;
    }
    // cache left and right
    auto const node_to_delete_left = node_to_delete->left;
    auto const node_to_delete_right = node_to_delete->right;
    // cache successor right
    auto const successor_right = successor->right;
    // overwrite children of successor
    successor->left = node_to_delete_left;
    // handle right is direct child of node to delete
    if ( node_to_delete_right == successor ) {
      // swap successor and node to delete
      successor->right = node_to_delete;
      node_to_delete->right = successor_right;
    } else {
      successor->right = node_to_delete_right;
      avl_node_t* successor_parent = successor_stack[ successor_top - 1 ];
      successor_parent->left = node_to_delete;
      node_to_delete->right = successor_right;
    }
    node_to_delete->left = nullptr;
    // remove node itself
    if ( path_top == 0 ) {
      root = successor;
    } else {
      avl_node_t* parent = path[ path_top - 1 ];
      if ( parent->left == node_to_delete ) {
        parent->left = successor;
      } else {
        parent->right = successor;
      }
    }
    // swap heights
    auto const tmp_height = node_to_delete->height;
    node_to_delete->height = successor->height;
    successor->height = tmp_height;
    // push successor to stack
    path[ path_top++ ] = successor;
    // parents downwards
    if ( node_to_delete_right != successor ) {
      for ( uint32_t i = 0; i < successor_top; i++ ) {
        path[ path_top++ ] = successor_stack[ i ];
      }
    }
    // remove node at the very end of the path
    auto const parent = path[ path_top - 1 ];
    if ( parent->left == node_to_delete ) {
      parent->left = successor_right;
    } else {
      parent->right = successor_right;
    }
  }
  // balance from top to bottom
  while ( path_top > 0 ) {
    // get last sub node
    auto const sub_node = path[ --path_top ];
    // balance it
    avl_node_t* balanced_node = balance( sub_node );
    // handle balanced node is not sub node
    if ( balanced_node != sub_node ) {
      // handle remaining nodes
      if ( path_top > 0 ) {
        // get parent
        avl_node_t* parent = path[ path_top - 1 ];
        // change left / right to balanced node
        if ( parent->left == sub_node ) {
          parent->left = balanced_node;
        } else {
          parent->right = balanced_node;
        }
        // change root node
      } else {
        root = balanced_node;
      }
    }
  }

  return root;
}

/**
 * @fn void push_output_level(uint8_t)
 * @brief Push something to depth buffer
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
 * @fn void pop_output_level(void)
 * @brief Pop something from depth
 */
static void pop_output_level( void ) {
  level_index -= 4;
  level_buffer[ level_index ] = 0;
}

/**
 * @fn void print_recursive(const avl_node_t*, avl_print_func_t)
 * @brief Recursive print of tree
 * @param node node to print
 * @param print printing function
 */
static void print_recursive( const avl_node_t* node, const avl_print_func_t print ) {
  if ( ! node ) {
    return;
  }
  // print information
  if ( ! print ) {
    if ( level_index ) {
      printf( "%s `--%"PRIx64"\r\n", ( const char* )level_buffer, node->data );
    } else {
      printf( "%"PRIx64"\r\n", node->data );
    }
  } else {
    if ( level_index ) {
      printf( "%s `--", ( const char* )level_buffer );
      print( ( avl_node_t* )node );
    } else {
      print( ( avl_node_t* )node );
    }
  }
  // go to left
  if ( node->left ) {
    push_output_level( '|' );
    print_recursive( node->left, print );
    pop_output_level();
  }
  // go to right
  if ( node->right ) {
    push_output_level( '|' );
    print_recursive( node->right, print );
    pop_output_level();
  }
}

/**
 * @fn int32_t avl_default_lookup(const avl_node_t*, uint64_t)
 * @brief Default lookup if not passed during creation
 * @param a
 * @param b
 * @return int32_t
 */
int32_t avl_default_lookup( const avl_node_t* a, const uint64_t b ) {
  return a->data == b ? 0 : ( a->data > b ? -1 : 1 ) ;
}

/**
 * @fn void avl_default_cleanup(avl_node_t*)
 * @brief Default cleanup if not passed during creation
 * @param a
 */
void avl_default_cleanup( [[maybe_unused]] avl_node_t* a ) {}

/**
 * @fn avl_tree_t avl_create_tree*(avl_compare_func_t, avl_lookup_func_t, avl_cleanup_func_t)
 * @brief Helper to create new tree
 * @param compare compare function to be used within tree
 * @param lookup
 * @param cleanup
 * @return avl_tree_t* pointer to new tree
 */
avl_tree_t* avl_create_tree(
  const avl_compare_func_t compare,
  const avl_lookup_func_t lookup,
  const avl_cleanup_func_t cleanup
) {
  // reserve space for new tree structure
  auto const new_tree = ( avl_tree_t* )malloc( sizeof( avl_tree_t ) );
  // check
  if ( !new_tree ) {
    return nullptr;
  }
  // prepare structure
  memset( ( void* )new_tree, 0, sizeof( avl_tree_t ) );

  // fill structure itself
  new_tree->root = nullptr;
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
 * @fn avl_node_t avl_create_node*(uint64_t)
 * @brief creates and prepares a avl node
 * @param data node data
 * @return avl_node_t*
 */
avl_node_t* avl_create_node( const uint64_t data ) {
  // reserve space for new node
  auto const node = ( avl_node_t* )malloc( sizeof( avl_node_t ) );
  // check
  if ( ! node ) {
    return nullptr;
  }
  // prepare data
  memset( ( void* )node, 0, sizeof( avl_node_t ) );
  // call prepare node
  avl_prepare_node( node, data );
  // return created node
  return node;
}

/**
 * @fn void avl_destroy_tree(avl_tree_t*)
 * @brief Helper to destroy created tree
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
 * @fn bool avl_insert_by_node(avl_tree_t*, avl_node_t*)
 * @brief Insert node into existing tree
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
 * @fn avl_node_t avl_find_by_data*(const avl_tree_t*, uint64_t)
 * @brief Find an avl node within tree
 * @param tree tree to search
 * @param data data to lookup
 * @return avl_node_t* found node or nullptr
 */
avl_node_t* avl_find_by_data( const avl_tree_t* tree, const uint64_t data ) {
  // end point
  if ( ! tree || ! tree->root ) {
    return nullptr;
  }
  auto current = tree->root;
  while ( current ) {
    // check result
    const int32_t result = tree->lookup( current, data );
    // handle match
    if ( 0 == result ) {
      return current;
    }
    if ( -1 == result ) {
      current = current->left;
    } else {
      current = current->right;
    }
  }
  return nullptr;
}

/**
 * @fn avl_node_t balance*(avl_node_t*)
 * @brief Method to balance node with return of new root node
 * @param node
 * @return avl_node_t*
 */
avl_node_t* balance( avl_node_t* node ) {
  // get balance factor
  const int32_t balance = balance_factor( node );

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
 * @fn avl_node_t avl_get_max*(avl_node_t*)
 * @brief Get max node of tree
 * @param root root to get max node
 * @return avl_node_t* found node or nullptr if empty
 */
avl_node_t* avl_get_max( avl_node_t* root ) {
  while ( root ) {
    if ( ! root->right ) {
      return root;
    }
    root = root->right;
  }
  return nullptr;
}

/**
 * @fn avl_node_t avl_get_min*(avl_node_t*)
 * @brief Get min node of tree
 * @param root node to get min value
 * @return avl_node_t* found node or nullptr if empty
 */
avl_node_t* avl_get_min( avl_node_t* root ) {
  while ( root ) {
    if ( ! root->left ) {
      return root;
    }
    root = root->left;
  }
  return nullptr;
}

/**
 * @fn void avl_prepare_node(avl_node_t*, uint64_t)
 * @brief method to prepare some node
 * @param node node to prepare
 * @param data initial node data
 */
void avl_prepare_node( avl_node_t* node, const uint64_t data ) {
  node->left = nullptr;
  node->right = nullptr;
  node->data = data;
  node->height = 0;
}

/**
 * @fn void avl_print(const avl_tree_t*, avl_print_func_t)
 * @brief Debug output avl tree
 * @param tree tree to dump
 * @param print function to use for printing
 */
void avl_print( const avl_tree_t* tree, const avl_print_func_t print ) {
  if ( ! tree->root ) {
    printf( "( empty tree )\r\n" );
    return;
  }
  print_recursive( tree->root, print );
}

/**
 * @fn void avl_remove_by_node(avl_tree_t*, avl_node_t*)
 * @brief Removes an avl tree by node
 * @param tree tree to work on
 * @param node node to remove
 */
void avl_remove_by_node( avl_tree_t* tree, avl_node_t* node ) {
  tree->root = remove_by_node( tree, node, tree->root );
}
