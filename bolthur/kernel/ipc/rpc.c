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

#include <string.h>
#include <stdlib.h>
#include <ipc/rpc.h>
#include <ipc/message.h>
#include <panic.h>
#if defined( PRINT_RPC )
  #include <debug/debug.h>
#endif

list_manager_ptr_t rpc_list = NULL;

/**
 * @fn int32_t rpc_container_lookup(const list_item_ptr_t, const void*)
 * @brief Helper to lookup an entry
 *
 * @param a
 * @param data
 * @return
 */
static int32_t rpc_container_lookup(
  const list_item_ptr_t a,
  const void* data
) {
  rpc_container_ptr_t container = a->data;
  return strcmp( container->identifier, data );
}

/**
 * @fn void rpc_container_cleanup(const list_item_ptr_t)
 * @brief Cleanup helper
 *
 * @param a
 */
static void rpc_container_cleanup( const list_item_ptr_t a ) {
  rpc_container_ptr_t container = a->data;
  // free name and destroy inner list
  free( container->identifier );
  list_destruct( container->handler );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn int32_t rpc_entry_lookup(const list_item_ptr_t, const void*)
 * @brief Helper to lookup an entry
 *
 * @param a
 * @param data
 * @return
 */
static int32_t rpc_entry_lookup(
  const list_item_ptr_t a,
  const void* data
) {
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "a->data = %#x, data = %#x\r\n", a->data, data )
  #endif
  rpc_entry_ptr_t entry = a->data;
  return entry->proc == data ? 0 : 1;
}

/**
 * @fn void rpc_entry_cleanup(const list_item_ptr_t)
 * @brief Cleanup helper
 *
 * @param a
 */
static void rpc_entry_cleanup( const list_item_ptr_t a ) {
  // free data entry
  free( a->data );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn void rpc_container_cleanup(const list_item_ptr_t)
 * @brief Cleanup helper
 *
 * @param a
 */
static void rpc_queue_cleanup( const list_item_ptr_t a ) {
  rpc_backup_ptr_t queue = a->data;
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Freeing up %#p\r\n", queue )
  #endif
  // free context
  if ( queue->context ) {
    free( queue->context );
  }
  if ( queue->message_id ) {
    message_remove( queue->thread->process->id, queue->message_id );
  }
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn bool rpc_init(void)
 * @brief Prepare rpc system
 *
 * @return
 */
bool rpc_init( void ) {
  rpc_list = list_construct( rpc_container_lookup, rpc_container_cleanup );
  return true;
}

/**
 * @fn bool rpc_register_handler(void)
 * @brief register a process handler
 *
 * @return
 */
bool rpc_register_handler(
  char* identifier,
  task_process_ptr_t proc,
  uintptr_t handler
) {
  // get container
  list_item_ptr_t container_item = list_lookup_data( rpc_list, identifier );
  rpc_container_ptr_t container = NULL;
  // add if not existing
  if ( ! container_item ) {
    // allocate new container
    container = malloc( sizeof( rpc_container_t ) );
    if ( ! container ) {
      return false;
    }
    // allocate space for name and copy it
    container->identifier = malloc(
      sizeof( char ) * ( strlen( identifier ) + 1 )
    );
    if ( ! container->identifier ) {
      free( container );
      return false;
    }
    strcpy( container->identifier, identifier );
    // construct list
    container->handler = list_construct( rpc_entry_lookup, rpc_entry_cleanup );
    if ( ! container->handler ) {
      free( container->identifier );
      free( container );
      return false;
    }
    // add to managing list
    if ( ! list_push_back( rpc_list, container ) ) {
      list_destruct( container->handler );
      free( container->identifier );
      free( container );
      return false;
    }
  } else {
    container = container_item->data;
  }
  // check for existing process mapping
  list_item_ptr_t proc_item = list_lookup_data( container->handler, proc );
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "proc_item = %#x\r\n", proc_item )
  #endif
  if ( proc_item ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT(
        "Process %d has already registered handler with identifier %s!\r\n",
        proc->id, identifier )
    #endif
    return false;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Registering handler for process %d\r\n", proc->id )
  #endif
  // generate new entry mapping
  rpc_entry_ptr_t entry = malloc( sizeof( rpc_entry_t ) );
  if ( ! entry ) {
    return false;
  }
  memset( entry, 0, sizeof( rpc_entry_t ) );
  // fill entry with data
  entry->handler = handler;
  entry->proc = proc;
  // generate queue
  entry->queue = list_construct( NULL, rpc_queue_cleanup );
  if ( ! entry->queue ) {
    free( entry );
    return false;
  }
  // push new entry to list
  if ( ! list_push_back( container->handler, entry ) ) {
    list_destruct( entry->queue );
    free( entry );
    return false;
  }
  // find
  return true;
}

/**
 * @fn bool rpc_unregister_handler(char*, task_process_ptr_t, uintptr_t)
 * @brief Unregister a handler
 *
 * @param identifier
 * @param proc
 * @param handler
 * @return
 */
bool rpc_unregister_handler(
  char* identifier,
  task_process_ptr_t proc,
  uintptr_t handler
) {
  // get container
  list_item_ptr_t container_item = list_lookup_data( rpc_list, identifier );
  rpc_container_ptr_t container = NULL;
  // add if not existing
  if ( ! container_item ) {
    return true;
  }
  // container
  container = container_item->data;
  // check for existing process mapping
  list_item_ptr_t proc_item = list_lookup_data( container->handler, proc );
  if ( ! proc_item ) {
    return true;
  }
  // check handler
  rpc_entry_ptr_t entry = proc_item->data;
  if ( entry->handler != handler ) {
    return false;
  }
  // remove and return success state of removal
  return list_remove( container->handler, proc_item );
}

/**
 * @fn rpc_backup_ptr_t rpc_raise(char*, task_thread_ptr_t, task_process_ptr_t, void*, size_t)
 * @brief Raise an rpc in target from source
 *
 * @param identifier
 * @param source
 * @param target
 * @param data
 * @param length
 */
rpc_backup_ptr_t rpc_raise(
  char* identifier,
  task_thread_ptr_t source,
  task_process_ptr_t target,
  void* data,
  size_t length
) {
  // get container
  list_item_ptr_t container_item = list_lookup_data( rpc_list, identifier );
  // add if not existing
  if ( ! container_item ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No container found for identifier %s\r\n", identifier )
    #endif
    // skip if no such container is existing
    return NULL;
  }
  // container
  rpc_container_ptr_t container = container_item->data;
  // check for existing process mapping
  list_item_ptr_t proc_item = list_lookup_data( container->handler, target );
  if ( ! proc_item ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "No handler bound for target %d\r\n", target->id )
    #endif
    // skip if nothing is there
    return NULL;
  }
  // get information entry
  rpc_entry_ptr_t entry = proc_item->data;

  // backup necessary stuff
  rpc_backup_ptr_t backup = rpc_create_backup( source, target, data, length );
  if ( ! backup ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Error while creating backup for target %d\r\n", target->id )
    #endif
    // skip if backup could not be created
    return NULL;
  }
  // prepare thread
  if ( ! rpc_prepare_invoke( backup, entry ) ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Error while preparing target %d\r\n", target->id )
    #endif
    free( backup->context );
    free( backup );
    // skip if error occurred during rpc invoke
    return NULL;
  }

  return backup;
}

/**
 * @fn rpc_backup_ptr_t rpc_get_active(task_thread_ptr_t)
 * @brief Get active rpc backup
 *
 * @param thread
 * @return
 */
rpc_backup_ptr_t rpc_get_active( task_thread_ptr_t thread ) {
  // ensure proper states
  if (
    TASK_THREAD_STATE_RPC_ACTIVE != thread->state
    || TASK_PROCESS_STATE_RPC_ACTIVE != thread->process->state
  ) {
    return false;
  }
  // variables
  list_item_ptr_t current = rpc_list->first;
  rpc_backup_ptr_t backup = NULL;
  // try to find active rpc
  while( current && ! backup ) {
    rpc_container_ptr_t container = current->data;
    // check for existing process mapping
    list_item_ptr_t proc_item = list_lookup_data(
      container->handler,
      thread->process
    );
    // handle existing
    if ( proc_item ) {
      rpc_entry_ptr_t entry = proc_item->data;
      list_item_ptr_t current_queued = entry->queue->first;
      while ( current_queued && ! backup ) {
        rpc_backup_ptr_t tmp = current_queued->data;
        // check for match
        if ( tmp->active ) {
          backup = tmp;
          break;
        }
        // get to next
        current_queued = current_queued->next;
      }
    }
    // switch to next
    current = current->next;
  }
  return backup;
}
