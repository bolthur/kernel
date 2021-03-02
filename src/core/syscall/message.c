
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

#include <stdlib.h>
#include <string.h>
#include <core/syscall.h>
#include <core/message.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif
#include <core/panic.h>

#include <inttypes.h>

/**
 * @brief Create message queue
 *
 * @param context
 */
void syscall_message_create( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_create()\r\n" )
  #endif
  // handle already set
  if( task_thread_current_thread->process->message_queue ) {
    syscall_populate_single_return( context, false );
    return;
  }
  // prepare message queue
  task_thread_current_thread->process->message_queue = list_construct( NULL, NULL );
  // handle error
  if ( ! task_thread_current_thread->process->message_queue ) {
    syscall_populate_single_return( context, false );
  }
  // return success
  syscall_populate_single_return( context, true );
}

/**
 * @brief Delete message queue
 *
 * @param context
 */
void syscall_message_destroy( __unused void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_destroy()\r\n" )
  #endif
  // destroy message queue
  list_destruct( task_thread_current_thread->process->message_queue );
}

/**
 * @brief Send message
 *
 * @param context
 */
void syscall_message_send_by_pid( void* context ) {
  // get parameter
  pid_t pid = ( pid_t )syscall_get_parameter( context, 0 );
  const char* data = ( const char* )syscall_get_parameter( context, 1 );
  size_t len = ( size_t )syscall_get_parameter( context, 2 );
  size_t sender_message_id = ( size_t )syscall_get_parameter( context, 3 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_message_send_by_pid( %d, %#p, %zx )\r\n", pid, data, len )
  #endif
  // get process by pid
  task_process_ptr_t target = task_process_get_by_id( pid );
  // handle error
  if ( ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process not found!\r\n" )
    #endif
      // set return and exit
      syscall_populate_single_return( context, 0 );
      return;
  }
  // handle error
  if ( ! target->message_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, 0 );
    return;
  }
  // handle invalid length
  if ( 0 == len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length passed!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, 0 );
    return;
  }

  // allocate message structure
  message_entry_ptr_t message_entry = ( message_entry_ptr_t )malloc(
    sizeof( message_entry_t ) );
  if ( ! message_entry ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No free space left for message structure!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, 0 );
    return;
  }

  // allocate message
  char* message = ( char* )malloc( len );
  if ( ! message ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No free space left for message data!\r\n" )
    #endif
    free( message_entry );
    // set return and exit
    syscall_populate_single_return( context, 0 );
    return;
  }
  // copy over content
  memcpy( message, data, len );
  // prepare structure
  message_entry->data = message;
  message_entry->len = len;
  message_entry->sender = task_thread_current_thread->process->id;
  message_entry->message_id = message_generate_id();
  message_entry->source_message_id = sender_message_id;
  // push message to process queue
  list_push_back( target->message_queue, message_entry );
  // return success
  syscall_populate_single_return( context, message_entry->message_id );
}

/**
 * @brief Send message by name
 *
 * @param context
 */
void syscall_message_send_by_name( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  const char* data = ( const char* )syscall_get_parameter( context, 1 );
  size_t len = ( size_t )syscall_get_parameter( context, 2 );
  size_t sender_message_id = ( size_t )syscall_get_parameter( context, 3 );
  size_t message_id = 0;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_send_by_name( %s, %#p, %zx )\r\n", name, data, len )
  #endif
  // handle invalid length
  if ( 0 == len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length passed!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, 0 );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "fetching list of processes with name \"%s\"\r\n", name )
  #endif
  // get name list
  list_manager_ptr_t name_list = task_process_get_by_name( name );
  if ( ! name_list ) {
    syscall_populate_single_return( context, 0 );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "name_list = %#x!\r\n", name_list )
  #endif
  // iterate
  list_item_ptr_t item = name_list->first;
  while( item ) {
    // convert pushed data to process
    task_process_ptr_t proc = ( task_process_ptr_t )item->data;
    // handle error
    if ( ! proc->message_queue ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Target process has no queue!\r\n" )
      #endif
      item = item->next;
      continue;
    }
    // allocate message structure
    message_entry_ptr_t message_entry = ( message_entry_ptr_t )malloc(
      sizeof( message_entry_t ) );
    if ( ! message_entry ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "No free space left for message structure!\r\n" )
      #endif
      item = item->next;
      continue;
    }
    // allocate message
    char* message = ( char* )malloc( len );
    if ( ! message ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "No free space left for message data!\r\n" )
      #endif
      free( message_entry );
      item = item->next;
      continue;
    }
    // copy over content
    memcpy( message, data, len );
    // prepare structure
    message_entry->data = message;
    message_entry->len = len;
    message_entry->sender = task_thread_current_thread->process->id;
    message_entry->source_message_id = sender_message_id;
    // push message to process queue
    if ( ! list_push_back( proc->message_queue, message_entry ) ) {
      // debug output
      #if defined( PRINT_SYSCALL )
        DEBUG_OUTPUT( "Unable to push message to queue!\r\n" )
      #endif
      free( message_entry );
      item = item->next;
      continue;
    }
    // generate id
    if ( 0 == message_id ) {
      message_id = message_generate_id();
    }
    // finally set message id
    message_entry->message_id = message_id;
    // next item
    item = item->next;
  }
  // return error
  syscall_populate_single_return( context, message_id );
}

/**
 * @brief receive message
 *
 * @param context
 */
void syscall_message_receive( void* context ) {
  // get parameter
  char* target = ( char* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  pid_t* pid = ( pid_t* )syscall_get_parameter( context, 2 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_receive( %#"PRIxPTR", %zx )\r\n", target, len )
  #endif

  // handle error
  if ( ! task_thread_current_thread->process->message_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, false );
    return;
  }
  // handle invalid length
  if ( 0 == len || ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length or target passed!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, false );
    return;
  }
  // get first message
  message_entry_ptr_t message_entry = ( message_entry_ptr_t )list_pop_front(
    task_thread_current_thread->process->message_queue
  );
  // handle no entry
  if ( ! message_entry ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No message existing!\r\n" )
    #endif
    syscall_populate_single_return( context, false );
    return;
  }
  // check for length mismatch
  if ( message_entry->len > len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        message_entry->len,
        len );
    #endif
    syscall_populate_single_return( context, false );
    return;
  }
  // copy over message content
  memcpy( target, message_entry->data, message_entry->len );
  if ( pid ) {
    *pid = message_entry->sender;
  }
  // return success
  syscall_populate_single_return( context, true );
}

/**
 * @brief Get response to a message by sent message id
 *
 * @param context
 */
void syscall_message_receive_response( void* context ) {
  // parameters
  char* target = ( char* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  size_t message_id = ( size_t )syscall_get_parameter( context, 2 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_message_receive_response( %#"PRIxPTR", %zx, %zu )\r\n",
      target, len, message_id )
  #endif

  // handle error
  if ( ! task_thread_current_thread->process->message_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, false );
    return;
  }
  // handle invalid length
  if ( 0 == len || ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length or target passed!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, false );
    return;
  }
  // loop through messages for a matching answer
  list_item_ptr_t item = NULL;
  message_entry_ptr_t found = NULL;
  // set start
  item = task_thread_current_thread->process->message_queue->first;
  // loop until match or end
  while( item ) {
    // get message item
    message_entry_ptr_t message_entry = ( message_entry_ptr_t )item->data;
    // check message
    if ( message_entry->source_message_id == message_id ) {
      found = message_entry;
      break;
    }
    // get next
    item = item->next;
  }
  // handle nothing
  if ( ! found ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No response found for message!\r\n" )
    #endif
    // set return and exit
    syscall_populate_single_return( context, false );
    return;
  }
  // check for length mismatch
  if ( found->len > len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        message_entry->len,
        len );
    #endif
    syscall_populate_single_return( context, false );
    return;
  }
  // remove list element
  if ( ! list_remove_data(
    task_thread_current_thread->process->message_queue,
    ( void* )found
  ) ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Error during remove of message from queue!\r\n" );
    #endif
    syscall_populate_single_return( context, false );
    return;
  }
  // copy over message content
  memcpy( target, found->data, found->len );
  // return success
  syscall_populate_single_return( context, false );
}
