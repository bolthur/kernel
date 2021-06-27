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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tar.h>
#include <core/panic.h>
#include <core/initrd.h>
#include <core/event.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/mm/shared.h>
#if defined( PRINT_PROCESS )
  #include <core/debug/debug.h>
#endif
#include <core/task/queue.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#include <core/task/stack.h>
#include <core/message.h>

/**
 * @brief Process management structure
 */
task_manager_ptr_t process_manager = NULL;

/**
 * @brief Compare id callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t process_compare_id(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %zu, b->data = %zu\r\n",
      ( size_t )a->data,
      ( size_t )b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( ( pid_t )a->data > ( pid_t )b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( pid_t )b->data > ( pid_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Compare id callback necessary for avl tree lookup
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t process_lookup_id(
  const avl_node_ptr_t a,
  const void* b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %zu, b->data = %zu\r\n", ( size_t )a->data, b );
  #endif
  // -1 if address of a is greater than address of b
  if ( ( pid_t )a->data > ( pid_t )b) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( pid_t )b > ( pid_t )a->data ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @brief Compare name callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t process_compare_name(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  task_process_name_ptr_t node_a = TASK_PROCESS_GET_BLOCK_NAME( a );
  task_process_name_ptr_t node_b = TASK_PROCESS_GET_BLOCK_NAME( b );
  // return comparison
  return strcmp( node_a->name, node_b->name );
}

/**
 * @brief Compare name callback necessary for avl tree lookup
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t process_lookup_name(
  const avl_node_ptr_t a,
  const void* b
) {
  task_process_name_ptr_t node_a = TASK_PROCESS_GET_BLOCK_NAME( a );
  // return comparison
  return strcmp( node_a->name, ( char* ) b );
}

/**
 * @fn void task_process_free(task_process_ptr_t, task_process_name_ptr_t)
 * @brief Helper to destroy process structure
 *
 * @param proc process structure to free
 * @param name name entry list
 */
static void task_process_free(
  task_process_ptr_t proc,
  task_process_name_ptr_t name
) {
  // handle invalid
  if ( ! proc ) {
    return;
  }
  // remove node
  avl_remove_by_node( process_manager->process_id, &proc->node_id );
  // destroy context if existing
  if ( proc->virtual_context ) {
    // unmap all mapped shared memory areas
    assert( shared_memory_cleanup_process( proc ) );
    // destroy context
    assert( virt_destroy_context( proc->virtual_context, false ) );
  }
  // set to null
  proc->virtual_context = NULL;
  // destroy thread manager
  if ( proc->thread_manager ) {
    task_thread_destroy( proc->thread_manager );
  }
  // destroy stack manager
  if ( proc->thread_stack_manager ) {
    task_stack_manager_destroy( proc->thread_stack_manager );
  }
  // destroy message queue
  if ( proc->message_queue ) {
    list_destruct( proc->message_queue );
  }
  // free up name
  if ( proc->name ) {
    free( proc->name );
  }
  // cleanup from name entry if set
  if ( name ) {
    list_remove_data( name->process, proc );
  }
  // free finally structure itself
  free( proc );
}

/**
 * @fn task_process_name_ptr_t task_process_get_name_list(const char*)
 * @brief Helper to get or create process name list
 *
 * @param name process name to get list of
 * @return
 */
task_process_name_ptr_t task_process_get_name_list( const char* name ) {
  // get possible name node
  avl_node_ptr_t name_node = avl_find_by_data(
    process_manager->process_name, ( void* )name );
  task_process_name_ptr_t name_entry = NULL;
  if ( ! name_node ) {
    name_entry = ( task_process_name_ptr_t )malloc(
      sizeof( task_process_name_t ) );
    // handle error
    if ( ! name_entry ) {
      return NULL;
    }
    // clear
    memset( name_entry, 0, sizeof( task_process_name_t ) );
    // allocate name
    name_entry->name = ( char* )malloc( ( strlen( name ) + 1 ) * sizeof( char ) );
    if ( ! name_entry->name ) {
      free( name_entry );
      return NULL;
    }
    name_entry->name = strcpy( name_entry->name, name );
    // create process list
    name_entry->process = list_construct( NULL, NULL );
    if ( ! name_entry->process ) {
      free( name_entry->name );
      free( name_entry );
      return NULL;
    }
    // push back
    if ( ! avl_insert_by_node(
      process_manager->process_name,
      &name_entry->node_name
    ) ) {
      list_destruct( name_entry->process );
      free( name_entry->name );
      free( name_entry );
      return NULL;
    }
  } else {
    name_entry = TASK_PROCESS_GET_BLOCK_NAME( name_node );
  }
  // return found or created name entry
  return name_entry;
}

/**
 * @brief Initialize task process manager
 * @return true
 * @return false
 */
bool task_process_init( void ) {
  // check parameter
  if ( process_manager ) {
    return false;
  }

  // allocate management structures
  process_manager = ( task_manager_ptr_t )malloc( sizeof( task_manager_t ) );
  // check parameter
  if ( ! process_manager ) {
    return false;
  }
  // prepare structure
  memset( ( void* )process_manager, 0, sizeof( task_manager_t ) );

  // create tree for managing processes by id
  process_manager->process_id = avl_create_tree(
    process_compare_id, process_lookup_id, NULL );
  // handle error
  if ( ! process_manager->process_id ) {
    free( process_manager );
    return false;
  }
  // create thread queue tree
  process_manager->thread_priority = task_queue_init();
  // handle error
  if ( ! process_manager->thread_priority ) {
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // create tree for managing processes by name
  process_manager->process_name = avl_create_tree(
    process_compare_name, process_lookup_name, NULL );
  // handle error
  if ( ! process_manager->process_name ) {
    avl_destroy_tree( process_manager->process_id );
    avl_destroy_tree( process_manager->thread_priority );
    free( process_manager );
    return false;
  }
  // create cleanup list
  process_manager->process_to_cleanup = list_construct( NULL, NULL );
  if ( ! process_manager->process_to_cleanup ) {
    avl_destroy_tree( process_manager->process_name );
    avl_destroy_tree( process_manager->process_id );
    avl_destroy_tree( process_manager->thread_priority );
    free( process_manager );
    return false;
  }
  // create cleanup list
  process_manager->thread_to_cleanup = list_construct( NULL, NULL );
  if ( ! process_manager->thread_to_cleanup ) {
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->process_name );
    avl_destroy_tree( process_manager->process_id );
    avl_destroy_tree( process_manager->thread_priority );
    free( process_manager );
    return false;
  }

  // register process switch event
  if ( ! event_bind( EVENT_PROCESS, task_process_schedule, true ) ) {
    list_destruct( process_manager->thread_to_cleanup );
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->process_name );
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // register cleanup
  if ( ! event_bind( EVENT_PROCESS, task_process_cleanup, true ) ) {
    event_unbind( EVENT_PROCESS, task_process_schedule, true );
    list_destruct( process_manager->thread_to_cleanup );
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->process_name );
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  if ( ! event_bind( EVENT_PROCESS, task_thread_cleanup, true ) ) {
    event_unbind( EVENT_PROCESS, task_process_cleanup, true );
    event_unbind( EVENT_PROCESS, task_process_schedule, true );
    list_destruct( process_manager->thread_to_cleanup );
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->process_name );
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // return success
  return true;
}

/**
 * @brief Method to generate new process id
 *
 * @return size_t generated process id
 */
pid_t task_process_generate_id( void ) {
  // current pid
  static pid_t current = 0;
  // return new pid by simple increment
  return ++current;
}

/**
 * @fn task_process_ptr_t task_process_create(size_t, pid_t, const char*)
 * @brief Method to create new process
 *
 * @param priority process priority
 * @param parent parent process id
 * @param name process name
 * @return
 */
task_process_ptr_t task_process_create(
  size_t priority,
  pid_t parent,
  const char* name
) {
  // check manager
  if ( ! process_manager ) {
    return NULL;
  }

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_process_create( %zu, %d, %s ) called\r\n",
      priority, parent, name );
  #endif

  // allocate process structure
  task_process_ptr_t process = ( task_process_ptr_t )malloc(
    sizeof( task_process_t ) );
  // check allocation
  if ( ! process ) {
    return NULL;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Allocated process structure at %p\r\n", ( void* )process );
  #endif

  // prepare structure
  memset( ( void* )process, 0, sizeof( task_process_t ) );
  // allocate name
  process->name = ( char* )malloc( ( strlen( name ) + 1 ) * sizeof( char ) );
  if ( ! process->name ) {
    task_process_free( process, NULL );
    return NULL;
  }
  // copy name
  process->name = strcpy( process->name, name );
  // populate process structure
  process->id = task_process_generate_id();
  process->thread_manager = task_thread_init();
  // handle error
  if ( ! process->thread_manager ) {
    task_process_free( process, NULL );
    return NULL;
  }
  process->state = TASK_PROCESS_STATE_INIT;
  process->priority = priority;
  process->parent = parent;
  process->thread_stack_manager = task_stack_manager_create();
  // handle error
  if ( ! process->thread_stack_manager ) {
    task_process_free( process, NULL );
    return NULL;
  }
  // create context only for user processes
  process->virtual_context = virt_create_context( VIRT_CONTEXT_TYPE_USER );
  // handle error
  if ( ! process->virtual_context ) {
    task_process_free( process, NULL );
    return NULL;
  }

  // prepare node
  avl_prepare_node( &process->node_id, ( void* )process->id );
  // add process to tree
  if ( ! avl_insert_by_node( process_manager->process_id, &process->node_id ) ) {
    task_process_free( process, NULL );
    return NULL;
  }

  // get possible name node
  task_process_name_ptr_t name_entry = task_process_get_name_list(
    process->name
  );
  if ( ! name_entry ) {
    task_process_free( process, NULL );
    return NULL;
  }

  // push back process to name list
  if ( ! list_push_back( name_entry->process, process ) ) {
    task_process_free( process, name_entry );
    return NULL;
  }
  // return allocated process
  return process;
}

/**
 * @fn task_process_ptr_t task_process_fork(task_thread_ptr_t)
 * @brief Generate complete copy of process to fork
 * @param thread_calling calling thread containing process information
 * @return forked process structure or null
 */
task_process_ptr_t task_process_fork( task_thread_ptr_t thread_calling ) {
  // allocate new process structure
  task_process_ptr_t forked = ( task_process_ptr_t )malloc(
    sizeof( task_process_t )
  );
  if ( ! forked ) {
    return NULL;
  }
  memset( ( void* )forked, 0, sizeof( task_process_t ) );
  task_process_ptr_t proc = thread_calling->process;

  // prepare dynamic data structures
  forked->name = ( char* )malloc( ( strlen( proc->name ) + 1 ) * sizeof( char ) );
  if ( ! forked->name ) {
    task_process_free( forked, NULL );
    return NULL;
  }
  forked->thread_manager = task_thread_init();
  if ( ! forked->thread_manager ) {
    task_process_free( forked, NULL );
    return NULL;
  }
  forked->thread_stack_manager = task_stack_manager_create();
  if ( ! forked->thread_stack_manager ) {
    task_process_free( forked, NULL );
    return NULL;
  }
  // fork virtual context
  forked->virtual_context = virt_fork_context( proc->virtual_context );
  if ( ! forked->virtual_context ) {
    task_process_free( forked, NULL );
    return NULL;
  }
  // create message queue if existing
  if( proc->message_queue ) {
    // prepare message queue
    forked->message_queue = list_construct( NULL, message_cleanup );
  }

  // populate data normal data
  forked->id = task_process_generate_id();
  forked->parent = proc->id;
  forked->state = TASK_PROCESS_STATE_READY;
  forked->priority = proc->priority;
  strcpy( forked->name, proc->name );

  // fork shared memory
  if ( ! shared_memory_fork( proc, forked ) ) {
    task_process_free( forked, NULL );
    return NULL;
  }

  // prepare node
  avl_prepare_node( &forked->node_id, ( void* )forked->id );
  // add process to tree
  if ( ! avl_insert_by_node( process_manager->process_id, &forked->node_id ) ) {
    task_process_free( forked, NULL );
    return NULL;
  }

  // get possible name node
  task_process_name_ptr_t name_entry = task_process_get_name_list(
    forked->name
  );
  if ( ! name_entry ) {
    task_process_free( forked, NULL );
    return NULL;
  }

  // push back process to name list
  if ( ! list_push_back( name_entry->process, forked ) ) {
    task_process_free( forked, name_entry );
    return NULL;
  }

  avl_node_ptr_t current = avl_iterate_first( proc->thread_manager );
  while ( current ) {
    // get thread
    task_thread_ptr_t thread = TASK_THREAD_GET_BLOCK( current );
    // try to fork it
    if ( ! task_thread_fork( forked, thread ) ) {
      task_process_free( forked, name_entry );
      return NULL;
    }
    // get next thread
    current = avl_iterate_next( proc->thread_manager, current );
  }

  return forked;
}

/**
 * @brief Resets process priority queues
 */
void task_process_queue_reset( void ) {
  // min / max queue
  task_priority_queue_ptr_t min_queue = NULL;
  task_priority_queue_ptr_t max_queue = NULL;
  avl_node_ptr_t min = NULL;
  avl_node_ptr_t max = NULL;

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "task_process_queue_reset()\r\n" );
  #endif

  // get min and max priority queue
  min = avl_get_min( process_manager->thread_priority->root );
  max = avl_get_max( process_manager->thread_priority->root );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "min: %p, max: %p\r\n", ( void* )min, ( void* )max );
  #endif

  // get nodes from min/max
  if ( min ) {
    min_queue = TASK_QUEUE_GET_PRIORITY( min );
  }
  if ( max ) {
    max_queue = TASK_QUEUE_GET_PRIORITY( max );
  }
  // handle no min or no max queue
  if ( ! min_queue || ! max_queue ) {
    return;
  }

  // loop through priorities and try to get next task
  for (
    size_t priority = max_queue->priority;
    priority >= min_queue->priority;
    priority--
  ) {
    // try to find queue for priority
    avl_node_ptr_t current_node = avl_find_by_data(
      process_manager->thread_priority,
      ( void* )priority );
    // skip if not existing
    if ( ! current_node ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if no such queue exists
      continue;
    }

    // get queue
    task_priority_queue_ptr_t current = TASK_QUEUE_GET_PRIORITY( current_node );
    // check for empty
    if ( list_empty( current->thread_list ) ) {
      // prevent endless loop by checking against 0
      if ( 0 == priority ) {
        break;
      }
      // skip if queue is handled
      continue;
    }

    // reset last handled
    current->last_handled = NULL;
    // prevent endless loop by checking against 0
    if ( 0 == priority ) {
      break;
    }
  }
}

/**
 * @fn void task_process_cleanup(event_origin_t, void*)
 * @brief Task process cleanup handling
 *
 * @param origin
 * @param context
 */
void task_process_cleanup(
  __unused event_origin_t origin,
  __unused void* context
) {
  list_item_ptr_t current = process_manager->process_to_cleanup->first;
  // loop
  while ( current ) {
    // get process from item
    task_process_ptr_t proc = ( task_process_ptr_t )current->data;

    // check for running thread
    avl_node_ptr_t current_thread = avl_iterate_first( proc->thread_manager );
    bool skip = false;
    while ( current_thread ) {
      // get thread
      task_thread_ptr_t thread = TASK_THREAD_GET_BLOCK( current_thread );
      // check for active
      if ( thread->state != TASK_THREAD_STATE_KILL ) {
        skip = true;
        break;
      }
      // get next thread
      current_thread = avl_iterate_next( proc->thread_manager, current_thread );
    }

    // skip
    if ( skip ) {
      // debug output
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "Skip process with id %d!\r\n", proc->id );
      #endif
      continue;
    }

    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Cleanup process with id %d!\r\n", proc->id );
    #endif
    // cleanup process
    task_process_free( proc, task_process_get_name_list( proc->name ) );

    // cache current as remove
    list_item_ptr_t remove = current;
    // head over to next
    current = current->next;

    // remove list item
    list_remove( process_manager->process_to_cleanup, remove );
  }
}

/**
 * @brief prepare init process by mapping ramdisk and device tree and adjusting initial context
 *
 * @param proc pointer to init process structure
 * @return bool true on success, else false
 *
 * @todo skip ramdisk if not existing
 * @todo skip device tree if not existing
 */
bool task_process_prepare_init( task_process_ptr_t proc ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "task_process_prepare_init( %p )\r\n", proc )
  #endif

  tar_header_ptr_t ramdisk = tar_lookup_file(
    initrd_get_start_address(),
    "ramdisk.tar.gz"
  );
  // handle error
  if ( ! ramdisk ) {
    return false;
  }
  // Get file address and size
  uintptr_t ramdisk_file = ( uintptr_t )tar_file( ramdisk );
  size_t ramdisk_file_size = tar_size( ramdisk );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "ramdisk file name %s\r\n", ramdisk->file_name )
    DEBUG_OUTPUT( "File size: %#zx\r\n", ramdisk_file_size )
  #endif
  // round up size
  size_t rounded_ramdisk_file_size = ROUND_UP_TO_FULL_PAGE( ramdisk_file_size );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "rounded size: %#zx\r\n", rounded_ramdisk_file_size )
  #endif
  // get physical area
  uint64_t phys_address_ramdisk = phys_find_free_page_range(
    PAGE_SIZE,
    rounded_ramdisk_file_size
  );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "phys address: %#llx\r\n", phys_address_ramdisk )
  #endif
  if( ! phys_address_ramdisk ) {
    return false;
  }
  // map temporary
  uintptr_t ramdisk_tmp = virt_map_temporary(
    phys_address_ramdisk,
    rounded_ramdisk_file_size
  );
  if ( !ramdisk_tmp ) {
    phys_free_page_range( phys_address_ramdisk, rounded_ramdisk_file_size );
    return false;
  }
  // copy over content
  memcpy( ( void* )ramdisk_tmp, ( void* )ramdisk_file, ramdisk_file_size );
  // unmap again
  virt_unmap_temporary( ramdisk_tmp, ( size_t )rounded_ramdisk_file_size );
  // find free page range
  uintptr_t proc_ramdisk_start = virt_find_free_page_range(
    proc->virtual_context,
    rounded_ramdisk_file_size,
    0
  );
  if ( ! proc_ramdisk_start ) {
    phys_free_page_range( phys_address_ramdisk, rounded_ramdisk_file_size );
    return false;
  }
  // map ramdisk
  if ( ! virt_map_address_range(
    proc->virtual_context,
    proc_ramdisk_start,
    phys_address_ramdisk,
    rounded_ramdisk_file_size,
    VIRT_MEMORY_TYPE_NORMAL,
    VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
  ) ) {
    phys_free_page_range( phys_address_ramdisk, rounded_ramdisk_file_size );
    return false;
  }

  int addr_size = sizeof( uintptr_t ) * 2;
  char str_ramdisk[ 20 ];
  char str_ramdisk_size[ 20 ];
  sprintf( str_ramdisk, "%#0*"PRIxPTR"\0", addr_size, proc_ramdisk_start );
  sprintf( str_ramdisk_size, "%#0*zx\0", addr_size, ramdisk_file_size );

  // get thread
  avl_node_ptr_t node = avl_iterate_first( proc->thread_manager );
  if ( ! node ) {
    return false;
  }
  task_thread_ptr_t thread = TASK_THREAD_GET_BLOCK( node );

  // empty env for init
  char* env[] = { NULL, };
  // arch related
  uintptr_t proc_additional_start = task_process_prepare_init_arch( proc );
  if ( proc_additional_start ) {
    char str_additional[ 20 ];
    sprintf(
      str_additional, "%#0*"PRIxPTR"\0", addr_size, proc_additional_start
    );

    char* arg[] = {
      "daemon:/init", str_ramdisk, str_ramdisk_size, str_additional, NULL, };
    assert( task_thread_push_arguments( thread, arg, env ) );
  } else {
    char* arg[] = { "daemon:/init", str_ramdisk, str_ramdisk_size, NULL, };
    assert( task_thread_push_arguments( thread, arg, env ) );
  }

  return true;
}

/**
 * @brief Get task structure by id
 *
 * @param pid
 * @return
 */
task_process_ptr_t task_process_get_by_id( pid_t pid ) {
  // lookup process id tree
  avl_node_ptr_t found = avl_find_by_data(
    process_manager->process_id,
    ( void* )pid
  );
  // handle not existing
  if ( ! found ) {
    return NULL;
  }
  // return found entry
  return TASK_PROCESS_GET_BLOCK_ID( found );
}

/**
 * @brief Get list of processes by name
 *
 * @param name
 * @return
 */
list_manager_ptr_t task_process_get_by_name( const char* name ) {
  avl_node_ptr_t found = avl_find_by_data(
    process_manager->process_name,
    ( void* )name
  );
  if ( ! found ) {
    return NULL;
  }
  return ( TASK_PROCESS_GET_BLOCK_NAME( found ) )->process;
}

/**
 * @fn void task_process_prepare_kill(void*, task_process_ptr_t)
 * @brief Prepare process kill
 *
 * @param context
 * @param proc
 */
void task_process_prepare_kill( void* context, task_process_ptr_t proc ) {
  // set process state
  proc->state = TASK_PROCESS_STATE_KILL;
  // push process to cleanup list
  list_push_back( process_manager->process_to_cleanup, proc );
  // trigger schedule and cleanup
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}
