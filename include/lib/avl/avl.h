
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

#if ! defined( __LIBAVL_AVL__ )
#define __LIBAVL_AVL__

#include <stdint.h>
#include <stddef.h>

// forward declarations
typedef struct avl_node avl_node_t;

// type declarations
typedef int avl_compare_func_t( const void* avl_a, const void* avl_b, void* avl_param );

typedef struct avl_node {
  void* data;

  int8_t balance;

  avl_node_t* left;
  avl_node_t* right;
} avl_node_t;

typedef struct avl_tree {
  avl_node_t* root;
  avl_compare_func_t* compare;
  void* param;
  // alloc
  size_t count;
} avl_tree_t;

avl_tree_t* avl_create( avl_compare_func_t*, void* );

#endif
