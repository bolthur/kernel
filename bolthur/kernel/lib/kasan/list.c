/**
 * Copyright (C) 2018 - 2025 bolthur project.
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
#include "list.h"

/**
 * @fn void kasan_list_init(kasan_list_t*)
 * @brief Initialize list
 * @param list
 */
__no_sanitize void kasan_list_init( kasan_list_t* list ) {
  list->next = NULL;
  list->prev = NULL;
}

/**
 * @fn void kasan_list_insert_tail(kasan_list_t*, kasan_list_t*)
 * @brief Insert into list at tail
 * @param list
 * @param item
 */
__no_sanitize void kasan_list_insert_tail( kasan_list_t* list, kasan_list_t* item ) {
  // cache list in entry
  kasan_list_t* entry = list;
  // loop to end
  while ( entry->next ) {
    entry = entry->next;
  }
  // set next of entry
  entry->next = item;
  // set previous of item
  item->prev = entry;
  // set next of item
  item->next = NULL;
}

/**
 * @fn bool kasan_list_empty(kasan_list_t*)
 * @brief Wrapper to check if list is empty
 * @param list
 * @return
 */
__no_sanitize bool kasan_list_empty( kasan_list_t* list ) {
  return !list->next && !list->prev;
}

/**
 * @fn kasan_list_t* kasan_list_shift(kasan_list_t*)
 * @brief Wrapper to shift list item from list
 * @param list
 * @return
 */
__no_sanitize kasan_list_t* kasan_list_shift( kasan_list_t* list ) {
  kasan_list_t* entry = list;
  // handle next existing
  if ( list->next ) {
    // set previous of next
    list->next->prev = entry->prev;
    // overwrite list
    list = list->next;
  }
  // unset entry next and previous
  entry->next = NULL;
  entry->prev = NULL;
  // return shifted entry
  return entry;
}

/**
 * @fn void kasan_list_remove(kasan_list_t*)
 * @brief Remove item from list
 * @param entry
 */
__no_sanitize void kasan_list_remove( kasan_list_t* entry ) {
  // cache previous and next of entry
  kasan_list_t* next = entry->next;
  kasan_list_t* prev = entry->prev;
  // handle previous existing
  if ( prev ) {
    prev->next = next;
  }
  // handle next existing
  if ( next ) {
    next->prev = prev;
  }
  // unset next and prev of entry
  entry->next = NULL;
  entry->prev = NULL;
}

/**
 * @fn kasan_list_t* kasan_list_find(kasan_list_t*, uintptr_t)
 * @brief Find address in list
 * @param list
 * @param addr
 * @return
 */
__no_sanitize kasan_list_t* kasan_list_find( kasan_list_t* list, uintptr_t addr ) {
  kasan_list_t* entry = list;
  // iterate through list
  while ( entry ) {
    // check for found
    if ( addr == ( uintptr_t )entry ) {
      return entry;
    }
    // go to next entry
    entry = entry->next;
  }
  // nothing found, so return null
  return NULL;
}
