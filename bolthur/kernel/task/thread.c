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

#include "../debug/debug.h"
#include "../lib/stdlib.h"
#include "../lib/string.h"
#include "../lib/inttypes.h"
#if defined( PRINT_PROCESS )
  #include "../debug/debug.h"
#endif
#include "../event.h"
#include "queue.h"
#include "thread.h"
#include "stack.h"
#include "../mm/virt.h"

/**
 * @brief Current running thread
 * @todo Transform to pointer to multiple threads ( depending on cpu size )
 */
task_thread_t* task_thread_current_thread = nullptr;

/**
 * @brief next thread to try to switch to
 */
task_thread_t* task_thread_try_switch_to = nullptr;

/**
 * @brief Task thread priority weight
 */
const uint32_t task_thread_priority_weight[ 40 ] = {
  /* -20 */ 88761, 71755, 56483, 46273, 36291,
  /* -15 */ 29154, 23254, 18705, 14949, 11916,
  /* -10 */  9548,  7620,  6100,  4904,  3906,
  /*  -5 */  3121,  2501,  1991,  1586,  1277,
  /*   0 */  1024,   820,   655,   526,   423,
  /*   5 */   335,   272,   215,   172,   137,
  /*  10 */   110,    87,    70,    56,    45,
  /*  15 */    36,    29,    23,    18,    15,
};

/**
 * @fn int32_t thread_compare_id_callback(const avl_node_t*, const avl_node_t*)
 * @brief Helper necessary for thread manager list
 *
 * @param a node a
 * @param b node b
 * @return
 */
static int32_t thread_compare_id_callback( const list_item_t* a, const void* b ) {
  auto const thread = ( task_thread_t* )a->data;
  if ( thread->id == ( pid_t )b ) {
    return 0;
  }
  return 1;
}

/**
 * @fn void thread_destroy_callback(list_item_t*)
 * @brief Helper to destroy list node
 *
 * @param node
 *
 * @todo check and revise function
 */
static void thread_destroy_callback( list_item_t* node ) {
  // get thread and context
  auto const thread = ( task_thread_t* )node->data;
  const task_process_t* proc = thread->process;
  virt_context_t* ctx = proc->virtual_context;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "Destroy thread with id %d of process with id %d!\r\n",
      thread->id,
      thread->process->id
    )
  #endif
  // unmap thread stack
  while (
    proc->virtual_context
    && ! virt_unmap_address_range( ctx, thread->stack_virtual - thread->stack_size, thread->stack_size, true )
  ) {
    // loop until successful unmapped!
  }
  // remove from scheduling tree
  task_queue_dequeue_specific( thread );
  // remove from stack address from manager
  while ( ! task_stack_manager_remove(
    thread->stack_virtual,
    proc->thread_stack_manager
  ) ) {
    // wait until removal was successful
  }
  // free context
  if ( thread->current_context ) {
    free( thread->current_context );
  }
  // free thread
  free( thread );
  // call default cleanup
  list_default_cleanup( node );
}

/**
 * @fn pid_t task_thread_generate_id(void)
 * @brief Method to generate new thread id
 * @return
 */
pid_t task_thread_generate_id( void ) {
  // return new pid by simple increment
  static pid_t current_thread_id = 0;
  return ++current_thread_id;
}

/**
 * @fn bool task_thread_set_current(task_thread_t*)
 * @brief Sets current running thread
 *
 * @param thread
 * @return
 */
bool task_thread_set_current( task_thread_t* thread ) {
  // check parameter
  if ( ! thread ) {
    return false;
  }
  // set current thread
  task_thread_current_thread = thread;
  // set state
  task_thread_set_state(
    task_thread_current_thread,
    task_thread_current_thread->state == TASK_THREAD_STATE_RPC_QUEUED
      ? TASK_THREAD_STATE_RPC_ACTIVE
      : TASK_THREAD_STATE_ACTIVE
  );
  // return success
  return true;
}

/**
 * @fn void task_thread_reset_current(void)
 * @brief Reset current thread
 */
void task_thread_reset_current( void ) {
  // set state
  if ( task_thread_current_thread ) {
    if ( TASK_THREAD_STATE_HALT_SWITCH == task_thread_current_thread->state ) {
      task_thread_set_state( task_thread_current_thread, TASK_THREAD_STATE_READY );
    } else if ( TASK_THREAD_STATE_RPC_HALT_SWITCH == task_thread_current_thread->state ) {
      task_thread_set_state( task_thread_current_thread, TASK_THREAD_STATE_RPC_QUEUED );
    }
    // push to tree if ready
    if (
      TASK_THREAD_STATE_READY == task_thread_current_thread->state
      || TASK_THREAD_STATE_RPC_QUEUED == task_thread_current_thread->state
    ) {
      task_queue_enqueue( task_thread_current_thread );
    }
  }
  // unset current thread
  task_thread_current_thread = nullptr;
}

/**
 * @fn list_manager_t* task_thread_init(void)
 * @brief Create thread manager for task
 *
 * @return
 */
list_manager_t* task_thread_init( void ) {
  return list_construct(
    thread_compare_id_callback,
    thread_destroy_callback,
    nullptr
  );
}

/**
 * @fn void task_thread_destroy(avl_tree_t*)
 * @brief Destroy thread manager tree
 *
 * @param tree
 */
void task_thread_destroy( avl_tree_t* tree ) {
  avl_destroy_tree( tree );
}

/**
 * @fn bool task_thread_is_ready(task_thread_t*)
 * @brief Helper to check if thread is ready for execution
 *
 * @param thread
 * @return
 */
bool task_thread_is_ready( const task_thread_t* thread ) {
  return
    TASK_THREAD_STATE_READY == thread->state
    || TASK_THREAD_STATE_HALT_SWITCH == thread->state
    || TASK_THREAD_STATE_RPC_QUEUED == thread->state
    || TASK_THREAD_STATE_RPC_HALT_SWITCH == thread->state;
}

/**
 * @fn bool task_thread_is_active(task_thread_t*)
 * @brief Helper to check if thread is active
 *
 * @param thread
 * @return
 */
bool task_thread_is_active( const task_thread_t* thread ) {
  return
    TASK_THREAD_STATE_ACTIVE == thread->state
    || TASK_THREAD_STATE_RPC_ACTIVE == thread->state;
}

/**
 * @fn void task_thread_kill(task_thread_t*, bool)
 * @brief Prepare kill of a thread
 * @param thread thread to push to kill handling
 * @param schedule flag to indicate scheduling
 */
void task_thread_kill( task_thread_t* thread, const bool schedule ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "Prepare kill of thread %d of process %d\r\n",
      thread->id,
      thread->process->id
    )
  #endif
  // set thread state and push thread to clean up list
  task_thread_set_state( thread, TASK_THREAD_STATE_KILL );
  list_push_back_data( process_manager->thread_to_cleanup, thread->process );
  // trigger schedule if necessary
  if ( schedule ) {
    event_enqueue( EVENT_PROCESS );
    event_enqueue( EVENT_PROCESS_CLEANUP );
  }
}

/**
 * @fn void task_thread_cleanup(event_origin_t, void*)
 * @brief thread cleanup handling
 *
 * @param origin
 * @param context
 */
void task_thread_cleanup(
  [[maybe_unused]] event_origin_t origin,
  [[maybe_unused]] void* context
) {
  if ( ! process_manager->thread_to_cleanup->first ) {
    return;
  }
  list_item_t* current = process_manager->thread_to_cleanup->first;
  // loop
  while ( current ) {
    // get process from item
    auto const thread = ( task_thread_t* )current->data;
    // skip running
    if ( thread->state != TASK_THREAD_STATE_KILL ) {
      continue;
    }
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "Cleanup thread with id %d of process with id %d!\r\n",
        thread->id,
        thread->process->id
      )
    #endif
    // cache current as remove
    list_item_t* remove = current;
    // head over to next
    current = current->next;
    // remove node from tree and cleanup
    task_process_t* process = thread->process;
    list_remove_data( process->thread_list, thread, true );
    // check if threads are empty
    if ( ! process->thread_list->first ) {
      list_item_t* match = list_lookup_data
        ( process_manager->process_to_cleanup,
        ( void* )process->id
      );
      // push process to clean up list
      if ( ! match ) {
        list_push_back_data( process_manager->process_to_cleanup, process );
      }
    }
    // remove list item
    list_remove_item( process_manager->thread_to_cleanup, remove, true );
  }
}

/**
 * @fn void task_thread_block(task_thread_t*, task_thread_state_t, task_state_data_t)
 * @brief Block a thread with specific state and data
 *
 * @param thread
 * @param state
 * @param data
 */
void task_thread_block(
  task_thread_t* thread,
  const task_thread_state_t state,
  const task_state_data_t data
) {
  // no block if state is not ready or active
  if (
    TASK_THREAD_STATE_READY != thread->state
    && TASK_THREAD_STATE_ACTIVE != thread->state
    && TASK_THREAD_STATE_RPC_ACTIVE != thread->state
  ) {
    return;
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "process = %d\r\n", thread->process->id )
  #endif
  // backup current state
  thread->state_backup = thread->state;
  if ( TASK_THREAD_STATE_ACTIVE == thread->state ) {
    thread->state_backup = TASK_THREAD_STATE_READY;
  } else if ( TASK_THREAD_STATE_RPC_ACTIVE == thread->state ) {
    thread->state_backup = TASK_THREAD_STATE_RPC_QUEUED;
  }
  // set state and data
  task_thread_set_state( thread, state );
  thread->state_data = data;
  // enqueue in wait queue
  task_queue_enqueue_blocked( thread );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "thread->state_backup = %d\r\n", thread->state_backup )
    DEBUG_OUTPUT( "thread->state = %d\r\n", thread->state )
  #endif
}

/**
 * @fn void task_thread_unblock(task_thread_t*, task_thread_state_t, task_state_data_t)
 * @brief Unblock a thread with set state passed as parameter
 *
 * @param thread
 * @param necessary_state
 * @param necessary_data
 */
void task_thread_unblock(
  task_thread_t* thread,
  const task_thread_state_t necessary_state,
  const task_state_data_t necessary_data
) {
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "process = %d\r\n", thread->process->id )
  #endif
  // validate state
  if ( necessary_state != thread->state ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "process %d with state %d, but one of the following"
        " are matching: %d / %d\r\n",
        thread->process->id,
        thread->state,
        necessary_state,
        TASK_THREAD_STATE_RPC_WAIT_FOR_RETURN
      )
    #endif
    return;
  }
  // validate data
  if ( thread->state_data.data_ptr != necessary_data.data_ptr ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "invalid data ptr attribute %p / %p\r\n",
        thread->state_data.data_ptr,
        necessary_data.data_ptr
      )
    #endif
    return;
  }
  // validate data size
  if ( thread->state_data.data_size != necessary_data.data_size ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT(
        "invalid data size attribute %zu / %zu\r\n",
        thread->state_data.data_size,
        necessary_data.data_size
      )
    #endif
    return;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "%d: thread->state = %d\r\n", thread->process->id, thread->state )
  #endif
  // set back to back up again
  task_thread_set_state( thread, thread->state_backup );
  if (
    TASK_THREAD_STATE_READY == thread->state
    || TASK_THREAD_STATE_RPC_QUEUED == thread->state_backup
  ) {
    // remove from queue
    task_queue_dequeue_blocked( thread );
    // prepare node and add again to schedule tree
    task_queue_enqueue( thread );
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "%d: thread->state = %d\r\n", thread->process->id, thread->state )
  #endif
}

/**
 * @fn task_thread_t* task_thread_get_blocked(task_thread_state_t, task_state_data_t)
 * @brief Get possible blocked thread
 *
 * @param necessary_thread_state
 * @param necessary_thread_data
 * @return
 */
task_thread_t* task_thread_get_blocked(
  const task_thread_state_t necessary_thread_state,
  const task_state_data_t necessary_thread_data
) {
  auto current = task_queue_get_first_blocked();
  while ( current ) {
    auto const thread = ( task_thread_t* )current->data;
    if (
      thread->state == necessary_thread_state
      && thread->state_data.data_ptr == necessary_thread_data.data_ptr
      && thread->state_data.data_size == necessary_thread_data.data_size
    ) {
      return thread;
    }
    current = current->next;
  }
  return nullptr;
}

/**
 * @fn void task_thread_set_state(task_thread_t*, task_thread_state_t)
 * @brief Wrapper to set thread state
 * @param thread
 * @param state
 */
void task_thread_set_state( task_thread_t* thread, const task_thread_state_t state ) {
  if ( ! thread ) {
    return;
  }
  #if defined( PRINT_PROCESS )
    void* returnAddress = __builtin_return_address(0);
    DEBUG_OUTPUT( "change thread for pid %d from %d to %d. Return address is: %p\r\n",
      thread->process ? thread->process->id : 0, thread->state, state,
      returnAddress )
  #endif
  thread->state = state;
}
