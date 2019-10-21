
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

#if ! defined( __LIB_LIST__ )
#define __LIB_LIST__

#include <stdbool.h>
#include <stdint.h>

// forward declaration
typedef struct list_item list_item_t, *list_item_ptr_t;

// generic list item
typedef struct list_item {
  void* data;
  list_item_ptr_t previous;
  list_item_ptr_t next;
} list_item_t, *list_item_ptr_t;

list_item_ptr_t* list_construct( void* );
void list_destruct( list_item_ptr_t* );
void list_push( list_item_ptr_t*, void* );
void* list_pop( list_item_ptr_t* );
list_item_ptr_t list_node_create( void* );

#endif
