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

#if ! defined( __LIST_H__ )
#define __LIST_H__

#include <stdbool.h>
#include <stdint.h>

// forward declaration
typedef struct list_item list_item_t;
typedef struct list_item *list_item_ptr_t;
typedef struct list_manager list_manager_t;
typedef struct list_manager *list_manager_ptr_t;

typedef int32_t ( *list_lookup_func_t )(
  const list_item_ptr_t a,
  const void* data
);
typedef void ( *list_cleanup_func_t )(
  const list_item_ptr_t a
);

// generic list item
struct list_item {
  void* data;
  list_item_ptr_t previous;
  list_item_ptr_t next;
};

struct list_manager {
  list_item_ptr_t first;
  list_item_ptr_t last;
  list_lookup_func_t lookup;
  list_cleanup_func_t cleanup;
};

list_manager_ptr_t list_construct( list_lookup_func_t, list_cleanup_func_t );
void list_destruct( list_manager_ptr_t );
bool list_empty( list_manager_ptr_t );
list_item_ptr_t list_lookup_data( list_manager_ptr_t, void* );
list_item_ptr_t list_lookup_item( list_manager_ptr_t, list_item_ptr_t );
bool list_push_front( list_manager_ptr_t, void* );
bool list_push_front_node( list_manager_ptr_t, list_item_ptr_t );
bool list_push_back( list_manager_ptr_t, void* );
bool list_push_back_node( list_manager_ptr_t, list_item_ptr_t );
void* list_pop_front( list_manager_ptr_t );
void* list_pop_back( list_manager_ptr_t );
void* list_peek_front( list_manager_ptr_t );
void* list_peek_back( list_manager_ptr_t );
list_item_ptr_t list_node_create( void* );
void list_print( list_manager_ptr_t );
bool list_remove( list_manager_ptr_t, list_item_ptr_t );
bool list_remove_data( list_manager_ptr_t, void* );

int32_t list_default_lookup( const list_item_ptr_t a, const void* );
void list_default_cleanup( const list_item_ptr_t );

#endif
