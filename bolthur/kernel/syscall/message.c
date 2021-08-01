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

#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <ipc/message.h>
#include <task/process.h>
#include <task/thread.h>
#if defined( PRINT_SYSCALL )
  #include <debug/debug.h>
#endif
#include <panic.h>
#include <mm/heap.h>

/**
 * @fn void syscall_message_create(void*)
 * @brief Syscall to setup message queue
 *
 * @param context
 */
void syscall_message_create( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_create()\r\n" )
  #endif
  // setup message for process
  if ( ! message_setup_process( task_thread_current_thread->process ) ) {
    syscall_populate_error( context, ( size_t )-EIO );
    return;
  }
}

/**
 * @fn void syscall_message_destroy(void*)
 * @brief Syscall to destroy message queue
 *
 * @param context
 */
void syscall_message_destroy( __unused void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_destroy()\r\n" )
  #endif
  // destroy process message queue
  message_destroy_process( task_thread_current_thread->process );
}

/**
 * @fn void syscall_message_send_by_pid(void*)
 * @brief Send a message to process by pid
 *
 * @param context
 */
void syscall_message_send_by_pid( void* context ) {
  // get parameter
  pid_t pid = ( pid_t )syscall_get_parameter( context, 0 );
  size_t type = ( size_t )syscall_get_parameter( context, 1 );
  const char* data = ( const char* )syscall_get_parameter( context, 2 );
  size_t len = ( size_t )syscall_get_parameter( context, 3 );
  size_t sender_message_id = ( size_t )syscall_get_parameter( context, 4 );
  size_t message_id = 0;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_message_send_by_pid( %d, %#p, %zx )\r\n", pid, data, len )
    DEBUG_OUTPUT( "message_id = %d\r\n", message_id )
  #endif

  // send message by pid
  int err = message_send_by_pid(
    pid, task_thread_current_thread->process->id,
    type, data, len, sender_message_id, &message_id
  );
  // handle error
  if ( err ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "errno = %d\r\n", err );
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-err );
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "message_id = %d\r\n", message_id )
  #endif
  // return success
  syscall_populate_success( context, message_id );
}

/**
 * @fn void syscall_message_send_by_name(void*)
 * @brief Send message by name
 *
 * @param context
 */
void syscall_message_send_by_name( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  size_t type = ( size_t )syscall_get_parameter( context, 1 );
  const char* data = ( const char* )syscall_get_parameter( context, 2 );
  size_t len = ( size_t )syscall_get_parameter( context, 3 );
  size_t sender_message_id = ( size_t )syscall_get_parameter( context, 4 );
  size_t message_id = 0;
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_send_by_name( %s, %#p, %zx )\r\n", name, data, len )
  #endif
  // send message by pid
  int err = message_send_by_name(
    name, task_thread_current_thread->process->id,
    type, data, len, sender_message_id, &message_id
  );
  // handle error
  if ( err ) {
    // set return and exit
    syscall_populate_error( context, ( size_t )-err );
    return;
  }
  // return success
  syscall_populate_success( context, message_id );
}

/**
 * @fn void syscall_message_receive(void*)
 * @brief receive first message
 *
 * @param context
 *
 * @todo Set process to state waiting for response when no message is existing
 * @todo Trigger scheduling
 */
void syscall_message_receive( void* context ) {
  // get parameter
  char* target = ( char* )syscall_get_parameter( context, 0 );
  size_t len = ( size_t )syscall_get_parameter( context, 1 );
  pid_t* pid = ( pid_t* )syscall_get_parameter( context, 2 );
  size_t* mid = ( size_t* )syscall_get_parameter( context, 3 );
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
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // handle invalid length
  if ( 0 == len || ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length or target passed!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get first message
  message_entry_ptr_t message_entry = ( message_entry_ptr_t )list_peek_front(
    task_thread_current_thread->process->message_queue
  );
  // handle no entry
  if ( ! message_entry ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No message existing!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMSG );
    return;
  }
  // check for length mismatch
  if ( message_entry->length > len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        message_entry->length,
        len );
    #endif
    syscall_populate_error( context, ( size_t )-EMSGSIZE );
    return;
  }
  // copy over message content
  memcpy( target, message_entry->data, message_entry->length );
  // copy pid if passed
  if ( pid ) {
    *pid = message_entry->sender;
  }
  // copy message id if passed
  if ( mid ) {
    *mid = message_entry->id;
  }
  // remove it
  list_remove_data(
    task_thread_current_thread->process->message_queue,
    ( void* )message_entry
  );
}

/**
 * @fn void syscall_message_receive_type(void*)
 * @brief Receive message type
 *
 * @param context
 *
 * @todo Set process to state waiting for response when nothing is in there
 * @todo Trigger scheduling
 */
void syscall_message_receive_type( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_receive_type()\r\n" )
  #endif

  // handle error
  if ( ! task_thread_current_thread->process->message_queue ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Target process has no queue!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // get first message
  message_entry_ptr_t message_entry = ( message_entry_ptr_t )list_peek_front(
    task_thread_current_thread->process->message_queue
  );
  // handle no entry
  if ( ! message_entry ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No message existing!\r\n" )
    #endif
    syscall_populate_error( context, ( size_t )-ENOMSG );
    return;
  }
  // return with message type
  syscall_populate_success( context, message_entry->type );
}

/**
 * @fn void syscall_message_wait_for_response(void*)
 * @brief Get response to a message by sent message id
 *
 * @param context
 */
void syscall_message_wait_for_response( void* context ) {
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
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // handle invalid length
  if ( 0 == len || ! target ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Invalid length or target passed!\r\n" )
    #endif
    // set return and exit
    syscall_populate_error( context, ( size_t )-EINVAL );
    return;
  }
  // loop through messages for a matching answer
  list_item_ptr_t item = NULL;
  message_entry_ptr_t found = NULL;
  // set start
  item = task_thread_current_thread->process->message_queue->first;
  // loop until match or end
  while( item && ! found ) {
    // get message item
    message_entry_ptr_t message_entry = ( message_entry_ptr_t )item->data;
    // check message and set found
    if ( message_entry->request == message_id ) {
      found = message_entry;
    }
    // get next
    item = item->next;
  }
  // handle nothing
  if ( ! found ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "No response found for message, blocking process with schedule!\r\n" )
    #endif
    task_thread_block(
      task_thread_current_thread,
      TASK_THREAD_WAITING_FOR_MESSAGE,
      ( task_state_data_t ){ .data_size = message_id }
    );
    // set return and exit
    syscall_populate_error( context, ( size_t )-ENOMSG );
    // enqueue schedule
    event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
    return;
  }
  // check for length mismatch
  if ( found->length > len ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT(
        "Not enough space for message existing ( %#zx - %#zx )!\r\n",
        found->length,
        len );
    #endif
    syscall_populate_error( context, ( size_t )-EMSGSIZE );
    return;
  }
  // copy over message content
  memcpy( target, found->data, found->length );
  // remove list element
  list_remove_data(
    task_thread_current_thread->process->message_queue,
    ( void* )found
  );
}

/**
 * @fn void syscall_message_wait_for_response_type(void*)
 * @brief Get response type to a message by sent message id
 *
 * @param context
 *
 * @todo Set process to state waiting for response
 * @todo Store message id the process is waiting for somehow
 * @todo Trigger scheduling
 * @todo Add logic
 * @todo Return errno on error
 */
void syscall_message_wait_for_response_type( void* context ) {
  // parameters
  /*__unused size_t message_type = ( size_t )syscall_get_parameter( context, 0 );
  __unused size_t message_id = ( size_t )syscall_get_parameter( context, 1 );*/
  // return type of message
  syscall_populate_error( context, ( size_t )-ENOSYS );
}

/**
 * @fn void syscall_message_wait_has_by_name(void*)
 * @brief Check for process existing with message box by name
 *
 * @param context
 */
void syscall_message_has_by_name( void* context ) {
  // get parameter
  const char* name = ( const char* )syscall_get_parameter( context, 0 );
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "syscall_message_has_by_name( %s )\r\n", name )
  #endif
  // get name list
  list_manager_ptr_t name_list = task_process_get_by_name( name );
  if ( ! name_list ) {
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // handle empty
  if ( ! name_list->first ) {
    syscall_populate_error( context, ( size_t )-ESRCH );
    return;
  }
  // check for same process
  list_item_ptr_t item = name_list->first;
  while( item ) {
    task_process_ptr_t proc = ( task_process_ptr_t )item->data;
    if( task_thread_current_thread->process->id == proc->id ) {
      // return success
      syscall_populate_error( context, ( size_t )-ESRCH );
      return;
    }
    item = item->next;
  }
  // return success
  syscall_populate_error( context, 0 );
}
