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
 * @fn message_entry_ptr_t message_allocate(size_t, char*, size_t*)
 * @brief Helper to allocate message
 *
 * @param message_length
 * @param message_data
 * @return
 */
message_entry_ptr_t message_allocate(
  size_t message_length,
  const char* message_data,
  size_t* message_id
) {
  // allocate message structure
  message_entry_ptr_t message = ( message_entry_ptr_t )malloc(
    sizeof( message_entry_t ) );
  if ( ! message ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "No free space left for message structure!\r\n" )
    #endif
    // return NULL
    return NULL;
  }
  // erase allocated space
  memset( message, 0, sizeof( message_entry_t ) );
  // allocate message data
  char* data = ( char* )malloc( message_length );
  if ( ! data ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "No free space left for message data!\r\n" )
    #endif
    // free allocation
    free( message );
    // return NULL
    return NULL;
  }
  // copy over content
  memcpy( data, message_data, message_length );
  // prepare necessary data
  message->data = data;
  message->length = message_length;
  // allocate message id
  if ( ! message_id || 0 == *message_id ) {
    message->id = message_generate_id();
    // set message id
    if ( message_id ) {
      *message_id = message->id;
    }
  } else {
    message->id = *message_id;
  }
  // return allocated structure
  return message;
}

/**
 * @fn int message_send(pid_t, pid_t, size_t, const char*, size_t, size_t, size_t*)
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
int message_send(
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
  if ( ! target_process ) {
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
  message_entry_ptr_t message = message_allocate(
    message_length,
    message_data,
    message_id
  );
  if ( ! message ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "No free space left for message structure!\r\n" )
    #endif
    return EIO;
  }
  // Save message id in pointer if not null
  if ( message_id ) {
    *message_id = message->id;
  }
  // prepare structure
  message->type = message_type;
  message->sender = sender;
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
      TASK_THREAD_STATE_MESSAGE_WAITING,
      ( task_state_data_t ){ .data_size = request_id }
    );
  }
  // return success
  return 0;
}

/**
 * @fn void message_remove(pid_t, size_t)
 * @brief Helper to remove message from queue by id
 *
 * @param process
 * @param message_id
 */
void message_remove( pid_t process, size_t message_id ) {
  // get process by pid
  task_process_ptr_t target_process = task_process_get_by_id( process );
  // handle error
  if ( ! target_process ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
    return;
  }
  // handle error
  if ( ! target_process->message_queue ) {
    // debug output
    #if defined( PRINT_MESSAGE )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    return;
  }
  // Get message by id
  list_item_ptr_t item = target_process->message_queue->first;
  message_entry_ptr_t found = NULL;
  while( item && ! found ) {
    // get message
    message_entry_ptr_t msg = ( message_entry_ptr_t )item->data;
    // set found when matching
    if( message_id == msg->id ) {
      found = msg;
    }
    // head over to next
    item = item->next;
  }
  // remove message if existing
  if ( found ) {
    list_remove_data( target_process->message_queue, ( void* )found );
  }
}
