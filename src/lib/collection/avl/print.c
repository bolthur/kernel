
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

#include <collection/avl.h>
#include <stdio.h>

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
 * @brief Debug output avl tree
 *
 * @param tree
 */
void avl_print( const avl_tree_ptr_t tree ) {
  print_recursive( tree->root );
}
