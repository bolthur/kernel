/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include "lib/stdlib.h"
#include "lib/string.h"
#include "lib/assert.h"
#include "timer.h"
#include "lib/collection/list.h"
#include "rpc/backup.h"
#include "rpc/generic.h"
#include "debug/debug.h"
#include "panic.h"
#if defined( PRINT_TIMER )
  #include "debug/debug.h"
#endif

list_manager_ptr_t timer_list;

/**
 * @fn size_t timer_generate_id(void)
 * @brief generate new callback id
 *
 * @return
 */
size_t timer_generate_id( void ) {
  // current pid
  static size_t current = 0;
  // return new pid by simple increment
  return ++current;
}

/**
 * @fn bool timer_insert(list_manager_ptr_t, void*)
 * @brief Timer insert callback
 *
 * @param list
 * @param data
 * @return
 */
static bool timer_insert(
  list_manager_ptr_t list,
  void* data
) {
  // push back if empty
  if ( list_empty( list ) ) {
    return list_push_back( list, data );
  }
  // get entry to add
  timer_callback_entry_ptr_t entry_to_add = ( timer_callback_entry_ptr_t )data;
  list_item_ptr_t item = list->first;
  while ( item ) {
    // get pointer to current entry
    timer_callback_entry_ptr_t entry = ( timer_callback_entry_ptr_t )item->data;
    // debug output
    #if defined( PRINT_TIMER )
      DEBUG_OUTPUT(
        "entry->expire = %d, entry_to_add->expire = %d\r\n",
        entry->expire, entry_to_add->expire
      )
    #endif
    // expire greater? => use previous one t
    if ( entry->expire > entry_to_add->expire ) {
      break;
    }
    // get to next item
    item = item->next;
  }
  // debug output
  #if defined( PRINT_TIMER )
    DEBUG_OUTPUT( "list = %p, item = %p, data = %p\r\n", list, item, data )
  #endif
  // in case there is an item to insert before, insert it
  if ( item ) {
    return list_insert_before( list, item, data );
  }
  // there is no match, so insert it behind
  return list_push_back( list, data );
}

/**
 * @fn void timer_cleanup(const list_item_ptr_t)
 * @brief timer cleanup callback
 *
 * @param item
 */
static void timer_cleanup( const list_item_ptr_t item ) {
  // free if data is valid
  if ( item->data ) {
    free( item->data );
  }
  // continue with default list cleanup
  list_default_cleanup( item );
}

/**
 * @fn int32_t timer_lookup(const list_item_ptr_t, const void*)
 * @brief
 *
 * @param a
 * @param data
 * @return
 */
static int32_t timer_lookup(
  const list_item_ptr_t a,
  const void* data
) {
  timer_callback_entry_ptr_t entry = a->data;
  return entry->id == ( size_t )data ? 0 : 1;
}

/**
 * @fn void timer_init(void)
 * @brief timer init stuff
 */
void timer_init( void ) {
  // construct list
  timer_list = list_construct( timer_lookup, timer_cleanup, timer_insert );
  assert( timer_list );
  // call platform init
  timer_platform_init();
}

/**
 * @fn timer_callback_entry_ptr_t timer_register_callback(task_thread_ptr_t, size_t, size_t)
 * @brief Register timer callback
 *
 * @param thread
 * @param rpc_num
 * @param timeout
 * @return
 */
timer_callback_entry_ptr_t timer_register_callback(
  task_thread_ptr_t thread,
  size_t rpc_num,
  size_t timeout
) {
  // allocate new entry structure
  timer_callback_entry_ptr_t entry = ( timer_callback_entry_ptr_t )malloc(
    sizeof( timer_callback_entry_t )
  );
  if ( ! entry ) {
    return NULL;
  }
  // push back information
  entry->rpc = rpc_num;
  entry->thread = thread;
  entry->expire = timeout;
  // generate id
  entry->id = timer_generate_id();
  // insert into ordered list
  if ( ! list_insert( timer_list, entry ) ) {
    #if defined( PRINT_TIMER )
      DEBUG_OUTPUT( "Timer insert failed!\r\n" )
    #endif
    free( entry );
    return NULL;
  }
  // return structure
  return entry;
}

/**
 * @fn bool timer_unregister_callback(size_t)
 * @brief Unregister timer callback by id
 *
 * @param id
 * @return
 */
bool timer_unregister_callback( size_t id ) {
  // try to find item
  list_item_ptr_t item = list_lookup_data( timer_list, ( void* ) id );
  if ( ! item ) {
    return true;
  }
  // remove item
  return list_remove( timer_list, item );
}

/**
 * @fn void timer_handle_callback(void)
 * @brief Handle expired timers
 */
void timer_handle_callback( void ) {
  // skip if list is empty
  if ( list_empty( timer_list ) ) {
    return;
  }
  // get current tick
  size_t tick = timer_get_tick();
  list_item_ptr_t current = timer_list->first;
  // loop through handles
  while( current ) {
    timer_callback_entry_ptr_t entry =
      ( timer_callback_entry_ptr_t )current->data;
    // debug output
    #if defined( PRINT_TIMER )
      DEBUG_OUTPUT( "tick = %zu, entry->expire = %zu\r\n", tick, entry->expire )
    #endif
    // break if tick is smaller than expire
    if ( entry->expire > tick ) {
      break;
    }
    // debug output
    #if defined( PRINT_TIMER )
      DEBUG_OUTPUT( "rpc = %zu, thread = %p, thread->process = %p\r\n",
        entry->rpc,
        entry->thread,
        entry->thread->process
      )
    #endif
    // raise rpc without data
    rpc_backup_ptr_t rpc = rpc_generic_raise(
      entry->thread,
      entry->thread->process,
      entry->rpc,
      NULL,
      0,
      entry->thread,
      false,
      0
    );
    // handle error by skip
    if ( ! rpc ) {
      // debug output
      #if defined( PRINT_TIMER )
        DEBUG_OUTPUT( "Unable to raise rpc\r\n" )
      #endif
      current = current->next;
      continue;
    }
    // cache current and set to next
    list_item_ptr_t to_remove = current;
    // switch to next
    current = current->next;
    // remove from list
    if ( ! list_remove( timer_list, to_remove ) ) {
      // debug output
      #if defined( PRINT_TIMER )
        DEBUG_OUTPUT( "Error while removing timer from list\r\n" )
      #endif
      // FIXME: remove 'rpc'
    }
  }
}
