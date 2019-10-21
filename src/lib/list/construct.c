
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

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <list.h>

#include <stdio.h>

list_item_ptr_t* list_construct( void* data ) {
  list_item_ptr_t node;
  list_item_ptr_t *list;

  // allocate list
  list = ( list_item_ptr_t* )malloc( sizeof( list_item_ptr_t ) );
  // assert malloc result
  assert( NULL != list );

  // allocate new node
  node = list_node_create( data );
  // set list pointer
  *list = node;

  // return created list
  return list;
}
