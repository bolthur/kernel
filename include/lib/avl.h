
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

#if ! defined( __LIB_AVL__ )
#define __LIB_AVL__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// forward declarations
typedef struct avl_node avl_node_t, *avl_node_ptr_t;
typedef struct avl_tree avl_tree_t, *avl_tree_ptr_t;

// type declarations
typedef int32_t ( *avl_compare_func_t )(
  const avl_node_ptr_t avl_a,
  const avl_node_ptr_t avl_b
);

typedef struct avl_node {
  void *data;

  avl_node_ptr_t left;
  avl_node_ptr_t right;
} avl_node_t, *avl_node_ptr_t;

typedef struct avl_tree {
  avl_node_ptr_t root;
  avl_compare_func_t compare;
} avl_tree_t, *avl_tree_ptr_t;

avl_node_ptr_t avl_get_max( const avl_node_ptr_t );
avl_node_ptr_t avl_get_min( const avl_node_ptr_t );
void avl_print( const avl_tree_ptr_t );
void avl_prepare_node( avl_node_ptr_t, void* );

avl_node_ptr_t avl_find_by_data( const avl_tree_ptr_t, void* );
avl_node_ptr_t avl_find_parent_by_data( const avl_tree_ptr_t, void* );
void avl_remove_by_data( const avl_tree_ptr_t, void* );

void avl_insert_by_node( const avl_tree_ptr_t, avl_node_ptr_t );
void avl_remove_by_node( const avl_tree_ptr_t, avl_node_ptr_t );

avl_tree_ptr_t avl_create_tree( avl_compare_func_t );
avl_node_ptr_t avl_create_node( void* );

avl_node_ptr_t balance( avl_node_ptr_t );

#endif
