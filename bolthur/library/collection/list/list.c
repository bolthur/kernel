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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "list.h"

/**
 * @fn int32_t list_default_lookup(const list_item_t*, const void*)
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
 * @fn void list_default_cleanup(list_item_t*)
 * @brief Default cleanup if not passed during creation
 *
 * @param a
 */
void list_default_cleanup( list_item_t* a ) {
  // free current element
  free( a );
}

/**
 * @fn bool list_default_insert(list_manager_t*, void*)
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
 * @fn list_manager_t list_construct*(list_lookup_func_t, list_cleanup_func_t, list_insert_func_t)
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
    return nullptr;
  }
  // overwrite with zero
  memset( ( void* )list, 0, sizeof( list_manager_t ) );

  // preset elements
  list->first = nullptr;
  list->last = nullptr;
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
 * @fn void list_destruct(list_manager_t*)
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
 * @fn bool list_empty(list_manager_t*)
 * @brief Method checks for list is empty
 *
 * @param list
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
 * @fn list_item_t list_lookup_data*(list_manager_t*, void*)
 * @brief Search a list item by data
 *
 * @param list list to lookup
 * @param data data to find
 * @return list_item_t*
 */
list_item_t* list_lookup_data( list_manager_t* list, void* data ) {
  // check parameter
  if ( !list ) {
    return nullptr;
  }

  // populate current
  list_item_t* current = list->first;

  // loop through list until end
  while ( current ) {
    if ( 0 == list->lookup( current, data ) ) {
      return current;
    }
    // check next one
    current = current->next;
  }

  // return not found
  return nullptr;
}

/**
 * @fn list_item_t list_lookup_item*(list_manager_t*, const list_item_t*)
 * @brief Search a list item by item
 *
 * @param list list to lookup
 * @param item item to find
 * @return list_item_t*
 */
list_item_t* list_lookup_item( list_manager_t* list, const list_item_t* item ) {
  // check parameter
  if ( !list ) {
    return nullptr;
  }

  // populate current
  list_item_t* current = list->first;

  // loop through list until end
  while ( current ) {
    if ( item == current ) {
      return current;
    }
    // check next one
    current = current->next;
  }

  // return not found
  return nullptr;
}

/**
 * @fn list_item_t list_item_create*(void*)
 * @brief Helper for creating a list node
 *
 * @param data data to populate
 * @return list_item_t* pointer to created node
 */
list_item_t* list_item_create( void* data ) {
  // allocate new node
  list_item_t* node = malloc( sizeof( list_item_t ) );
  // check malloc result
  if ( ! node ) {
    return nullptr;
  }
  // populate created node
  node->next = nullptr;
  node->previous = nullptr;
  node->data = data;
  // return created node
  return node;
}

/**
 * @fn void list_peek_front_data*(list_manager_t*)
 * @brief Method to get element from list like pop without removal
 *
 * @param list list to use
 * @return void* data of first element or nullptr if empty
 */
void* list_peek_front_data( list_manager_t* list ) {
  // handle invalid parameter or empty
  if ( ! list || ! list->first ) {
    return nullptr;
  }
  // return first element data
  return list->first->data;
}

/**
 * @fn void list_peek_back_data*(list_manager_t*)
 * @brief Method to get element from list like pop without removal
 *
 * @param list list to use
 * @return void* data of first element or nullptr if empty
 */
void* list_peek_back_data( list_manager_t* list ) {
  // check parameter
  if ( !list ) {
    return nullptr;
  }
  // get last element
  list_item_t* last = list->last;
  // handle empty list
  if ( !last ) {
    return nullptr;
  }
  // return data of first element
  return last->data;
}

/**
 * @fn void list_pop_front_data*(list_manager_t*)
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or nullptr if empty
 */
void* list_pop_front_data( list_manager_t* list ) {
  // check parameter
  if ( !list ) {
    return nullptr;
  }
  // get first element
  list_item_t* first = list->first;

  // handle empty list
  if ( !first ) {
    return nullptr;
  }

  // cache data of first element
  void* data = first->data;
  // change previous of next element if existing
  if ( first->next ) {
    // change previous
    first->next->previous = nullptr;
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
 * @fn void list_pop_back_data*(list_manager_t*)
 * @brief Method to pop element from list
 *
 * @param list list to use
 * @return void* data of first element or nullptr if empty
 */
void* list_pop_back_data( list_manager_t* list ) {
  // check parameter
  if ( !list ) {
    return nullptr;
  }
  // get last element
  list_item_t* last = list->last;

  // handle empty list
  if ( !last ) {
    return nullptr;
  }

  // cache data of first element
  void* data = last->data;
  // change next of previous element if existing
  if ( last->previous ) {
    // change previous
    last->previous->next = nullptr;
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
 * @fn void list_print(list_manager_t*)
 * @brief Method to print list
 *
 * @param list list to use
 */
void list_print( list_manager_t* list ) {
  // handle invalid
  if ( !list ) {
    return;
  }
  // populate current
  const list_item_t* current = list->first;

  // loop through list until end
  while ( current ) {
    printf( "list->data = %p\r\n", current->data );
    // get next element
    current = current->next;
  }
}

/**
 * @fn list_item_t* list_push_front_data(list_manager_t*, void*)
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param data data to push into list
 * @return true
 * @return false
 */
list_item_t* list_push_front_data( list_manager_t* list, void* data ) {
  // handle invalid parameter
  if ( ! list || ! data ) {
    return nullptr;
  }
  // set list head
  list_item_t* first = list->first;
  // create new node
  list_item_t* node = list_item_create(data);
  // handle error
  if ( !node ) {
    return nullptr;
  }
  // set next to first
  node->next = first;
  // set previous for first element
  if ( first ) {
    first->previous = node;
  }
  // overwrite first element within list pointer
  list->first = node;
  // set last element if invalid
  if ( ! list->last ) {
    list->last = list->first;
  }
  return node;
}

/**
 * @fn list_item_t* list_push_back_data(list_manager_t*, void*)
 * @brief Method to push node with data into list
 *
 * @param list list to use
 * @param data data to push into list
 * @return
 */
list_item_t* list_push_back_data( list_manager_t* list, void* data ) {
  // handle invalid parameter
  if ( !list || !data ) {
    return nullptr;
  }
  // create new node
  list_item_t* node = list_item_create( data );
  // handle error
  if ( ! node ) {
    return nullptr;
  }
  // handle empty
  if ( ! list->first ) {
    list->first = list->last = node;
    return node;
  }
  // set previous to last
  node->previous = list->last;
  // set next for last element
  list->last->next = node;
  // overwrite last element within list pointer
  list->last = node;
  // return success
  return node;
}

/**
 * @fn bool list_push_after_data(struct list_manager*, void*, void*)
 * @brief Function to insert data after data element if found
 * @param list list to manipulate
 * @param after data where new data shall be added as next
 * @param data data to be added
 * @return
 */
bool list_push_after_data( struct list_manager* list, void* after, void* data ) {
  // handle invalid parameter
  if ( ! list || ! data || ! after ) {
    return false;
  }
  // get item where to insert
  list_item_t* after_item = list_lookup_data( list, after );
  // handle not found
  if ( ! after_item ) {
    return false;
  }
  // create new node
  list_item_t* node = list_item_create( data );
  // handle error
  if ( !node ) {
    return false;
  }
  // insert after found item
  node->next = after_item->next;
  if ( after_item->next ) {
    after_item->next->previous = node;
  }
  node->previous = after_item;
  after_item->next = node;
  return true;
}

/**
 * @fn static bool list_remove_item_head(list_manager_t*)
 * @brief Helper to remove item from head
 * @param list
 * @return
 */
static bool list_remove_item_head( list_manager_t* list ) {
  if ( ! list->first ) {
    return false;
  }
  list->first = list->first->next;
  if ( list->first ) {
    list->first->previous = nullptr;
  } else {
    list->last = nullptr;
  }
  return true;
}

/**
 * @fn static bool list_remove_item_head(list_manager_t*)
 * @brief Helper to remove item from tail
 * @param list
 * @return
 */
static bool list_remove_item_tail( list_manager_t* list ) {
  if ( ! list->last ) {
    return false;
  }
  list->last = list->last->previous;
  if ( list->last ) {
    list->last->next = nullptr;
  } else {
    list->first = nullptr;
  }
  return true;
}

/**
 * @fn static bool list_remove_item_internal(list_manager_t*, list_item_t*)
 * @brief Helper to remove item internally
 * @param list
 * @param node
 * @return
 */
static bool list_remove_item_internal( list_manager_t* list, list_item_t* node ) {
  if ( ! list || ! node ) {
    return false;
  }
  bool result = false;
  if ( node == list->first ) {
    result = list_remove_item_head( list );
  } else if ( node == list->last ) {
    result = list_remove_item_tail( list );
  } else {
    node->previous->next = node->next;
    node->next->previous = node->previous;
    result = true;
  }
  return result;
}

/**
 * @fn bool list_remove_item(list_manager_t*, list_item_t*, bool)
 * @brief Remove list item
 *
 * @param list
 * @param item
 * @param cleanup
 * @return true
 * @return false
 */
bool list_remove_item( list_manager_t* list, list_item_t* item, const bool cleanup ) {
  // remove node
  const bool result = list_remove_item_internal( list, item );
  if ( ! result ) {
    return result;
  }
  // free list item
  if ( cleanup ) {
    list->cleanup( item );
  } else {
    item->next = item->previous = nullptr;
  }
  return true;
}

/**
 * @fn bool list_remove_data(list_manager_t*, void*, bool)
 * @brief Remove list item
 *
 * @param list
 * @param data
 * @param cleanup
 * @return true
 * @return false
 */
bool list_remove_data( list_manager_t* list, void* data, const bool cleanup ) {
  // handle invalid parameter
  if ( !list || !data ) {
    return false;
  }
  // stop if not existing
  list_item_t* item = list_lookup_data( list, data );
  if ( !item ) {
    return false;
  }
  // remove item
  const bool result = list_remove_item_internal( list, item );
  if ( ! result ) {
    return result;
  }
  // free list item
  if ( cleanup ) {
    list->cleanup( item );
  } else {
    item->next = item->previous = nullptr;
  }
  return true;
}

/**
 * @fn bool list_insert_data(list_manager_t*, void*)
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
 * @fn list_item_t* list_insert_data_before(list_manager_t*, list_item_t*, void*)
 * @brief Insert data before item
 *
 * @param list
 * @param item
 * @param data
 * @return
 */
list_item_t* list_insert_data_before(
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
    return nullptr;
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
  return to_insert;
}

/**
 * @fn list_item_t* list_insert_item_before(list_manager_t*, list_item_t*, list_item_t*)
 * @brief Wrapper to insert list item before specific item
 * @param list list to work on
 * @param item item where to insert before
 * @param to_insert item to insert
 * @return
 */
list_item_t* list_insert_item_before( list_manager_t* list, list_item_t* item, list_item_t* to_insert ) {
  if ( ! list || ! item || ! to_insert ) {
    return nullptr;
  }
  if ( list->first == item ) {
    list->first->previous = to_insert;
    to_insert->next = list->first;
    list->first = to_insert;
    return to_insert;
  }
  // cache previous
  list_item_t* previous = item->previous;
  // set next of previous
  previous->next = to_insert;
  // set previous of to insert
  to_insert->previous = previous;
  // set next of to insert
  to_insert->next = item;
  // set previous of item
  item->previous = to_insert;
  // return node
  return to_insert;
}

/**
 * @fn size_t list_count_item(list_manager_t*)
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
 * @fn list_item_t list_get_item_at_pos*(list_manager_t*, size_t)
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
    return nullptr;
  }
  // return item
  return current;
}
