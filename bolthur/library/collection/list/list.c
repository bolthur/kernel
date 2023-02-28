/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include <stdlib.h>
#include <stdio.h>
#include "list.h"

/**
 * @brief Default lookup if not passed during creation
 *
 * @param a
 * @param b
 * @return int32_t
 */
int32_t list_default_lookup( const list_item_t* a, const void* b ) {
  return a->data == b ? 0 : 1;
}

/**
 * @brief Default cleanup if not passed during creation
 *
 * @param a
 */
void list_default_cleanup( list_item_t* a ) {
  // free current element
  free( a );
}

/**
 * @brief Default insert callback
 *
 * @param list
 * @param data
 * @return
 */
bool list_default_insert( list_manager_t* list, void* data ) {
  return list_push_back_data( list, data );
}

/**
 * @brief Method to construct list
 *
 * @param lookup
 * @param cleanup
 * @param insert
 * @return list_manager_t* pointer to created list
 */
list_manager_t* list_construct(
  list_lookup_func_t lookup,
  list_cleanup_func_t cleanup,
  list_insert_func_t insert
) {
  // allocate list
  list_manager_t* list = malloc( sizeof( list_manager_t ) );
  // handle error
  if ( !list ) {
    return NULL;
  }
  // overwrite with zero
  memset( ( void* )list, 0, sizeof( list_manager_t ) );

  // preset elements
  list->first = NULL;
  list->last = NULL;
  // lookup function
  if ( lookup ) {
    list->lookup = lookup;
  } else {
    list->lookup = list_default_lookup;
  }
  // cleanup function
  if ( cleanup ) {
    list->cleanup = cleanup;
  } else {
    list->cleanup = list_default_cleanup;
  }
  // insert function
  if ( insert ) {
    list->insert = insert;
  } else {
    list->insert = list_default_insert;
  }

  // return created list
  return list;
}

/**
 * @brief Method to destruct list
 *
 * @param list list to use
 */
void list_destruct( list_manager_t* list ) {
  list_item_t* current;
  list_item_t* next;

  // check parameter
  if ( !list ) {
    return;
  }
  // populate current
  current = list->first;

  // loop through list until end
  while ( current ) {
    // get next element
    next = current->next;

    // additional cleanup
    list->cleanup( current );

    // overwrite current with next
    current = next;
  }

  // finally, free list
  free( list );
}

/**
 * @brief Method checks for list is empty
 *
 * @return true empty list
 * @return false at least one item
 */
bool list_empty( list_manager_t* list ) {
  // check parameter
  if ( !list ) {
    return true;
  }

  // check first and last item
  if ( !list->first && !list->last ) {
    // list empty
    return true;
  }

  // list not empty
  return false;
}

/**
 * @brief Search a list item by data
 *
 * @param list list to lookup
 * @param data data to find
 * @return list_item_t*
 */
list_item_t* list_lookup_data( list_manager_t* list, void* data ) {
  list_item_t* current;

  // check parameter
  if ( !list ) {
    return NULL;
  }
  // populate current
  current = list->first;

  // loop through list until end
  while ( current ) {
    if ( 0 == list->lookup( current, data ) ) {
      return current;
    }
    // check next one
    current = current->next;
  }

  // return not found
  return NULL;
}

/**
 * @brief Search a list item by item
 *
 * @param list list to lookup
 * @param item item to find
 * @return list_item_t*
 */
list_item_t* list_lookup_item( list_manager_t* list, const list_item_t* item ) {
  list_item_t* current;

  // check parameter
  if ( !list ) {
    return NULL;
  }
  // populate current
  current = list->first;


  // loop through list until end
  while ( current ) {
    if ( item == current ) {
      return current;
    }
    // check next one
    current = current->next;
  }

  // return not found
  return NULL;
}

/**
 * @brief Helper for creating a list node
 *
 * @param data data to populate
 * @return list_item_t* pointer to created node
 */
list_item_t* list_item_create( void* data ) {
  // allocate new node
  list_item_t* node = malloc( sizeof( list_item_t ) );
  // check malloc result
  if ( !node ) {
    return NULL;
  }
  // overwrite allocated memory with 0
  memset( ( void* )node, 0, sizeof( list_item_t ) );

  // populate created node
  node->next = NULL;
  node->previous = NULL;
  node->data = data;

  // return created node
  return node;
}

/**
 * @brief Method to get element from list like pop without removal
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_peek_front_data( list_manager_t* list ) {
  list_item_t* first;

  // check parameter
  if ( !list ) {
    return NULL;
  }
  // get first element
  first = list->first;

  // handle empty list
  if ( !first ) {
    return NULL;
  }
  // return data of first element
  return first->data;
}

/**
 * @brief Method to get element from list like pop without removal
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_peek_back_data( list_manager_t* list ) {
  list_item_t* last;

  // check parameter
  if ( !list ) {
    return NULL;
  }
  // get last element
  last = list->last;

  // handle empty list
  if ( !last ) {
    return NULL;
  }
  // return data of first element
  return last->data;
}

/**
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_pop_front_data( list_manager_t* list ) {
  void* data;
  list_item_t* first;

  // check parameter
  if ( !list ) {
    return NULL;
  }
  // get first element
  first = list->first;

  // handle empty list
  if ( !first ) {
    return NULL;
  }

  // cache data of first element
  data = first->data;
  // change previous of next element if existing
  if ( first->next ) {
    // change previous
    first->next->previous = NULL;
  }

  // change list to next to remove first element from list
  list->first = first->next;
  // change last if no next element is existing
  if ( !list->first || !list->first->next ) {
    list->last = list->first;
  }

  // free first element
  free( first );
  // return set data
  return data;
}

/**
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or NULL if empty
 */
void* list_pop_back_data( list_manager_t* list ) {
  void* data;
  list_item_t* last;

  // check parameter
  if ( !list ) {
    return NULL;
  }
  // get last element
  last = list->last;

  // handle empty list
  if ( !last ) {
    return NULL;
  }

  // cache data of first element
  data = last->data;
  // change next of previous element if existing
  if ( last->previous ) {
    // change previous
    last->previous->next = NULL;
  }

  // change list to next to remove first element from list
  list->last = last->previous;
  // change first if no next element is existing
  if ( !list->last || !list->last->previous ) {
    list->first = list->last;
  }

  // free first element
  free( last );
  // return set data
  return data;
}

/**
 * @brief Method to print list
 *
 * @param list list to use
 */
void list_print( list_manager_t* list ) {
  list_item_t* current;

  // handle invalid
  if ( !list ) {
    return;
  }
  // populate current
  current = list->first;

  // loop through list until end
  while ( current ) {
    printf( "list->data = %p\r\n", current->data );
    // get next element
    current = current->next;
  }
}

/**
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param data data to push into list
 * @return true
 * @return false
 */
bool list_push_front_data( list_manager_t* list, void* data ) {
  list_item_t* first;
  list_item_t* node;

  // handle invalid parameter
  if ( !list || !data ) {
    return false;
  }
  // set list head
  first = list->first;

  // create new node
  node = list_item_create( data );
  // handle error
  if ( !node ) {
    return false;
  }

  // set next to first
  node->next = first;
  // set previous for first element
  if ( first ) {
    first->previous = node;
  }

  // overwrite first element within list pointer
  list->first = node;
  // set last element if NULL
  if ( !list->last ) {
    list->last = list->first;
  }

  return true;
}

/**
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param data data to push into list
 * @return true
 * @return false
 */
bool list_push_back_data( list_manager_t* list, void* data ) {
  list_item_t* last;
  list_item_t* node;

  // handle invalid parameter
  if ( !list || !data ) {
    return false;
  }
  // set list head
  last = list->last;

  // create new node
  node = list_item_create( data );
  // handle error
  if ( !node ) {
    return false;
  }

  // set previous to last
  node->previous = last;
  // set next for last element
  if ( last ) {
    last->next = node;
  }

  // overwrite last element within list pointer
  list->last = node;
  // set first element if NULL
  if ( !list->first ) {
    list->first = list->last;
  }

  return true;
}

/**
 * @brief Remove list item
 *
 * @param list
 * @param item
 * @return true
 * @return false
 */
bool list_remove_item( list_manager_t* list, list_item_t* item ) {
  // handle invalid parameter
  if ( !list || !item ) {
    return false;
  }
  // stop if not existing
  if ( !list_lookup_item( list, item ) ) {
    return false;
  }

  // set previous of next
  if ( item->next ) {
    item->next->previous = item->previous;
  }

  // set next of previous
  if ( item->previous ) {
    item->previous->next = item->next;
  }

  // handle head removal
  if ( item == list->first ) {
    list->first = item->next;
  }
  // handle foot removal
  if ( item == list->last ) {
    list->last = item->previous;
  }

  // free list item
  list->cleanup( item );
  return true;
}

/**
 * @brief Remove list item
 *
 * @param list
 * @param data
 * @return true
 * @return false
 */
bool list_remove_data( list_manager_t* list, void* data ) {
  // handle invalid parameter
  if ( !list || !data ) {
    return false;
  }

  // stop if not existing
  list_item_t* item = list_lookup_data( list, data );
  if ( !item ) {
    return false;
  }

  // set previous of next
  if ( item->next ) {
    item->next->previous = item->previous;
  }

  // set next of previous
  if ( item->previous ) {
    item->previous->next = item->next;
  }

  // handle head removal
  if ( item == list->first ) {
    list->first = item->next;
  }
  // handle foot removal
  if ( item == list->last ) {
    list->last = item->previous;
  }

  // free list item
  list->cleanup( item );
  return true;
}

/**
 * @brief Insert data with insert callback
 *
 * @param list
 * @param data
 * @return
 */
bool list_insert_data( list_manager_t* list, void* data ) {
  return list->insert( list, data );
}

/**
 * @brief Insert data before item
 *
 * @param list
 * @param item
 * @param data
 * @return
 */
bool list_insert_data_before(
  list_manager_t* list,
  list_item_t* item,
  void* data
) {
  // handle item is first one
  if ( list->first == item ) {
    return list_push_front_data( list, data );
  }
  // create new node
  list_item_t* to_insert = list_item_create( data );
  // handle error
  if ( !to_insert ) {
    return false;
  }
  // cache previous item
  list_item_t* previous = item->previous;
  if ( previous ) {
    previous->next = to_insert;
    to_insert->previous = previous;
  }
  to_insert->next = item;
  item->previous = to_insert;
  // success
  return true;
}

/**
 * @brief Method to count list items
 *
 * @param list
 * @return
 */
size_t list_count_item( list_manager_t* list ) {
  size_t count = 0;
  // count all items
  for ( list_item_t* current = list->first; current; current = current->next ) {
    count++;
  }
  // return amount
  return count;
}

/**
 * @brief Helper to get item at position
 *
 * @param list
 * @param position
 * @return
 */
list_item_t* list_get_item_at_pos( list_manager_t* list, size_t position ) {
  size_t to_index = 0;
  list_item_t* current = list->first;
  // get parameter item
  while ( current && to_index < position ) {
    current = current->next;
    to_index++;
  }
  // handle mismatch / overflow
  if ( to_index != position ) {
    return NULL;
  }
  // return item
  return current;
}
