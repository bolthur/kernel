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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ipc/message.h>
#if defined( PRINT_MESSAGE )
  #include <debug/debug.h>
#endif

/**
 * @brief Init message queue stuff
 *
 * @return true
 * @return false
 */
bool message_init( void ) {
  return true;
}

/**
 * @brief list item cleanup helper
 *
 * @param item
 */
void message_cleanup( const list_item_ptr_t item ) {
  if ( item->data ) {
    // transform to entry
    const message_entry_ptr_t entry = ( message_entry_ptr_t )item->data;
    // free message if set
    if ( entry->data ) {
      free( ( void* )entry->data );
    }
    // free entry
    free( entry );
  }
  // continue with default list cleanup
  list_default_cleanup( item );
}

/**
 * @brief Generates new message id
 *
 * @return
 */
size_t message_generate_id( void ) {
  static size_t id = 1;
  return id++;
}

/**
 * @fn void message_setup_process(task_process_ptr_t)
 * @brief Method to setup message queue for process
 *
 * @param proc
 */
bool message_setup_process( task_process_ptr_t proc ) {
  // stop if already setup
  if ( proc->message_queue ) {
    return true;
  }
  // prepare message queue
  proc->message_queue = list_construct( NULL, message_cleanup );
  return proc->message_queue;
}

/**
 * @fn void message_destroy_process(task_process_ptr_t)
 * @brief Method to destroy message queue for process
 *
 * @param proc
 */
void message_destroy_process( task_process_ptr_t proc ) {
  // handle no message queue
  if ( ! proc->message_queue ) {
    return;
  }
  // destroy message queue
  list_destruct( proc->message_queue );
  // set to NULL after destroy
  proc->message_queue = NULL;
}

/**
 * @fn int message_send_by_pid(pid_t, pid_t, size_t, const char*, size_t, size_t, size_t*)
 * @brief Method to send message from one process to another one by pid
 *
 * @param target
 * @param sender
 * @param message_type
 * @param message_data
 * @param message_length
 * @param request_id
 * @param message_id
 * @return
 */
int message_send_by_pid(
  pid_t target,
  pid_t sender,
  size_t message_type,
  const char* message_data,
  size_t message_length,
  size_t request_id,
  size_t* message_id
) {
  // get process by pid
  task_process_ptr_t target_process = task_process_get_by_id( target );
  // handle error
  if ( ! target ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    return EINVAL;
  }
  // handle error
  if ( ! target_process->message_queue ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    return EINVAL;
  }
  // handle invalid length
  if ( 0 == message_length ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "Invalid length passed!\r\n" )
    #endif
    return EINVAL;
  }

  // allocate message structure
  message_entry_ptr_t message = ( message_entry_ptr_t )malloc(
    sizeof( message_entry_t ) );
  if ( ! message ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "No free space left for message structure!\r\n" )
    #endif
    return EIO;
  }

  // allocate message
  char* data = ( char* )malloc( message_length );
  if ( ! data ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "No free space left for message data!\r\n" )
    #endif
    free( message );
    return EIO;
  }
  // generate message id if existing
  if ( 0 == *message_id ) {
    *message_id = message_generate_id();
  }
  // copy over content
  memcpy( data, message_data, message_length );
  // prepare structure
  message->data = data;
  message->length = message_length;
  message->type = message_type;
  message->sender = sender;
  message->id = *message_id;
  message->request = request_id;
  // push message to process queue
  list_push_back( target_process->message_queue, message );
  // unblock thread if blocked
  if ( request_id ) {
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT(
        "Unblocking threads waiting for message with id %d\r\n",
        request_id )
    #endif
    task_unblock_threads(
      target_process,
      TASK_THREAD_WAITING_FOR_MESSAGE,
      ( task_state_data_t ){ .data_size = request_id }
    );
  }
  // return success
  return 0;
}

/**
 * @fn int message_send_by_name(const char*, pid_t, size_t, const char*, size_t, size_t, size_t*)
 * @brief Method to send a message to one or more other processes by name
 *
 * @param target
 * @param sender
 * @param message_type
 * @param message_data
 * @param message_length
 * @param request_id
 * @param message_id
 * @return
 */
int message_send_by_name(
  const char* target,
  pid_t sender,
  size_t message_type,
  const char* message_data,
  size_t message_length,
  size_t request_id,
  size_t* message_id
) {
  // handle invalid length
  if ( 0 == message_length ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "Invalid length passed!\r\n" )
    #endif
    return EINVAL;
  }
  // debug output
  #if defined( PRINT_MESSAGE )
    DEBUG_OUTPUT( "fetching list of processes with name \"%s\"\r\n", target )
  #endif
  // get name list
  list_manager_ptr_t name_list = task_process_get_by_name( target );
  if ( ! name_list ) {
    return EINVAL;
  }
  // debug output
  #if defined( PRINT_MESSAGE )
    DEBUG_OUTPUT( "name_list = %#x!\r\n", name_list )
  #endif
  // iterate
  list_item_ptr_t item = name_list->first;
  *message_id = message_generate_id();
  while( item ) {
    // convert pushed data to process
    task_process_ptr_t proc = ( task_process_ptr_t )item->data;
    // send by pid
    int err = message_send_by_pid(
      proc->id,
      sender,
      message_type,
      message_data,
      message_length,
      request_id,
      message_id
    );
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT(
        "receiver = %d, sender = %d message_id = %d, request_id = %d\r\n",
        proc->id, sender, *message_id, request_id )
    #endif
    if ( err ) {
      // debug output
      #if defined( PRINT_MESSAGE )
        DEBUG_OUTPUT( "No free space left for message data!\r\n" )
      #endif
      item = item->next;
      continue;
    }
    // unblock thread if blocked
    if ( 0 < request_id ) {
      task_unblock_threads(
        proc,
        TASK_THREAD_WAITING_FOR_MESSAGE,
        ( task_state_data_t ){ .data_size = request_id }
      );
    }
    // next item
    item = item->next;
  }
  // return success
  return 0;
}
