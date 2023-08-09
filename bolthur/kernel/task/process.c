/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include "../lib/inttypes.h"
#include "../lib/assert.h"
#include "../lib/stdlib.h"
#include "../lib/string.h"
#include "../lib/stdio.h"
#include "../lib/tar.h"
#include "../lib/duplicate.h"
#include "../interrupt.h"
#include "../elf.h"
#include "../panic.h"
#include "../initrd.h"
#include "../event.h"
#include "../mm/phys.h"
#include "../mm/virt.h"
#include "../mm/shared.h"
#if defined( PRINT_PROCESS )
  #include "../debug/debug.h"
#endif
#include "queue.h"
#include "process.h"
#include "thread.h"
#include "stack.h"
#include "../rpc/data.h"
#include "../rpc/queue.h"
#include "../rpc/generic.h"

/**
 * @brief Process management structure
 */
task_manager_t* process_manager = NULL;

/**
 * @fn int32_t process_compare_id(const avl_node_t*, const avl_node_t*)
 * @brief Compare id callback necessary for avl tree
 *
 * @param a
 * @param b
 * @return
 */
static int32_t process_compare_id(
  const avl_node_t* a,
  const avl_node_t* b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", a, b )
    DEBUG_OUTPUT(
      "a->data = %zu, b->data = %zu\r\n",
      ( size_t )a->data,
      ( size_t )b->data
    )
  #endif

  // -1 if address of a->data is greater than address of b->data
  if ( ( pid_t )a->data > ( pid_t )b->data ) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
  } else if ( ( pid_t )b->data > ( pid_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @fn int32_t process_lookup_id(const avl_node_t*, const void*)
 * @brief Compare id callback necessary for avl tree lookup
 *
 * @param a
 * @param b
 * @return
 */
static int32_t process_lookup_id(
  const avl_node_t* a,
  const void* b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", a, b )
    DEBUG_OUTPUT(
      "a->data = %d, b->data = %d\r\n",
      ( pid_t )a->data,
      ( pid_t )b
    )
  #endif
  // -1 if address of a->data is greater than address of b->data
  if ( ( pid_t )a->data > ( pid_t )b) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
  } else if ( ( pid_t )b > ( pid_t )a->data ) {
    return 1;
  }
  // equal => return 0
  return 0;
}

/**
 * @fn void task_process_free(task_process_t*)
 * @brief Helper to destroy process structure
 *
 * @param proc process structure to free
 */
static void task_process_free( task_process_t* proc ) {
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
  // destroy rpc stuff
  rpc_generic_destroy( proc );
  // free finally structure itself
  free( proc );
}

/**
 * @fn int32_t cleanup_process_lookup_id(const list_item_t*, const void*)
 * @brief Compare id callback necessary for cleanup list
 *
 * @param a
 * @param data
 * @return
 */
static int32_t cleanup_process_lookup_id(
  const list_item_t* a,
  const void* data
) {
  task_process_t* process = a->data;
  return process->id == ( pid_t )data ? 0 : 1;
}

/**
 * @fn void_t cleanup_process_delete(list_item_t*)
 * @brief cleanup process list delete helper
 *
 * @param a
 * @return
 */
static void cleanup_process_delete( list_item_t* a ) {
  task_process_t* process = a->data;
  task_process_free( process );
  list_default_cleanup( a );
}

/**
 * @fn bool task_process_init(void)
 * @brief Initialize task process manager
 *
 * @return
 */
bool task_process_init( void ) {
  // check parameter
  if ( process_manager ) {
    return false;
  }

  // reserve space for management structures
  process_manager = malloc( sizeof( *process_manager ) );
  // check parameter
  if ( ! process_manager ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "error while reserving space for process manager\r\n" )
    #endif
    return false;
  }
  // prepare structure
  memset( ( void* )process_manager, 0, sizeof( *process_manager ) );

  // create tree for managing processes by id
  process_manager->process_id = avl_create_tree(
    process_compare_id, process_lookup_id, NULL );
  // handle error
  if ( ! process_manager->process_id ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "process id tree failed\r\n" )
    #endif
    free( process_manager );
    return false;
  }
  // create thread queue tree
  process_manager->thread_priority = task_queue_init();
  // handle error
  if ( ! process_manager->thread_priority ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "thread_priority failed\r\n" )
    #endif
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // create cleanup list
  process_manager->process_to_cleanup = list_construct(
    cleanup_process_lookup_id,
    cleanup_process_delete,
    NULL
  );
  if ( ! process_manager->process_to_cleanup ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "process_to_cleanup failed\r\n" )
    #endif
    avl_destroy_tree( process_manager->process_id );
    avl_destroy_tree( process_manager->thread_priority );
    free( process_manager );
    return false;
  }
  process_manager->thread_to_cleanup = list_construct( NULL, NULL, NULL );
  if ( ! process_manager->thread_to_cleanup ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "thread_to_cleanup failed\r\n" )
    #endif
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->process_id );
    avl_destroy_tree( process_manager->thread_priority );
    free( process_manager );
    return false;
  }
  // register process switch event
  if ( ! event_bind( EVENT_PROCESS, task_process_schedule, true ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "bind failed\r\n" )
    #endif
    list_destruct( process_manager->thread_to_cleanup );
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // register cleanup
  if ( ! event_bind( EVENT_PROCESS, task_process_cleanup, true ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "bind failed\r\n" )
    #endif
    event_unbind( EVENT_PROCESS, task_process_schedule, true );
    list_destruct( process_manager->thread_to_cleanup );
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  if ( ! event_bind( EVENT_PROCESS, task_thread_cleanup, true ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "bind failed\r\n" )
    #endif
    event_unbind( EVENT_PROCESS, task_process_cleanup, true );
    event_unbind( EVENT_PROCESS, task_process_schedule, true );
    list_destruct( process_manager->thread_to_cleanup );
    list_destruct( process_manager->process_to_cleanup );
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // return success
  return true;
}

/**
 * @fn pid_t task_process_generate_id(void)
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
 * @fn task_process_t* task_process_create(size_t, pid_t)
 * @brief Method to create new process
 *
 * @param priority process priority
 * @param parent parent process id
 * @return
 */
task_process_t* task_process_create( size_t priority, pid_t parent ) {
  // check manager
  if ( ! process_manager ) {
    return NULL;
  }

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_process_create( %zu, %d ) called\r\n",
      priority,
      parent
    )
  #endif

  // reserve space for process structure
  task_process_t* process = malloc( sizeof( *process ) );
  // check
  if ( ! process ) {
    return NULL;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Reserved space for process structure at %p\r\n", process )
  #endif

  // prepare structure
  memset( ( void* )process, 0, sizeof( task_process_t ) );
  // populate process structure
  process->id = task_process_generate_id();
  process->thread_manager = task_thread_init();
  // handle error
  if ( ! process->thread_manager ) {
    task_process_free( process );
    return NULL;
  }
  process->priority = priority;
  process->parent = parent;
  process->thread_stack_manager = task_stack_manager_create();
  // handle error
  if ( ! process->thread_stack_manager ) {
    task_process_free( process );
    return NULL;
  }
  // create context only for user processes
  process->virtual_context = virt_create_context( VIRT_CONTEXT_TYPE_USER );
  // handle error
  if ( ! process->virtual_context ) {
    task_process_free( process );
    return NULL;
  }

  // prepare node
  avl_prepare_node( &process->node_id, ( void* )process->id );
  // add process to tree
  if ( ! avl_insert_by_node( process_manager->process_id, &process->node_id ) ) {
    task_process_free( process );
    return NULL;
  }
  // return process
  return process;
}

/**
 * @fn task_process_t* task_process_fork(task_thread_t*)
 * @brief Generate complete copy of process to fork
 *
 * @param thread_calling calling thread containing process information
 * @return forked process structure or null
 */
task_process_t* task_process_fork( task_thread_t* thread_calling ) {
  // reserve new process structure
  task_process_t* forked = malloc( sizeof( *forked ) );
  if ( ! forked ) {
    return NULL;
  }
  memset( ( void* )forked, 0, sizeof( task_process_t ) );
  task_process_t* proc = thread_calling->process;

  // prepare dynamic data structures
  forked->thread_manager = task_thread_init();
  if ( ! forked->thread_manager ) {
    task_process_free( forked );
    return NULL;
  }
  forked->thread_stack_manager = task_stack_manager_create();
  if ( ! forked->thread_stack_manager ) {
    task_process_free( forked );
    return NULL;
  }
  // fork virtual context
  forked->virtual_context = virt_fork_context( proc->virtual_context );
  if ( ! forked->virtual_context ) {
    task_process_free( forked );
    return NULL;
  }
  // create rpc queues if existing
  if ( proc->rpc_data_queue && ! rpc_data_queue_setup( forked ) ) {
    task_process_free( forked );
    return NULL;
  }
  // create message queue if existing
  if ( proc->rpc_queue && ! rpc_queue_setup( forked ) ) {
    task_process_free( forked );
    return NULL;
  }
  // copy rpc handler
  forked->rpc_handler = proc->rpc_handler;
  forked->rpc_ready = proc->rpc_ready;

  // populate data normal data
  forked->id = task_process_generate_id();
  forked->parent = proc->id;
  forked->priority = proc->priority;
  forked->current_thread_id = 0;

  // fork shared memory
  if ( ! shared_memory_fork( proc, forked ) ) {
    task_process_free( forked );
    return NULL;
  }

  // prepare node
  avl_prepare_node( &forked->node_id, ( void* )forked->id );
  // add process to tree
  if ( ! avl_insert_by_node( process_manager->process_id, &forked->node_id ) ) {
    task_process_free( forked );
    return NULL;
  }

  avl_node_t* current = avl_iterate_first( proc->thread_manager );
  while ( current ) {
    // get thread
    task_thread_t* thread = TASK_THREAD_GET_BLOCK( current );
    // try to fork it
    if ( ! task_thread_fork( forked, thread ) ) {
      task_process_free( forked );
      return NULL;
    }
    // get next thread
    current = avl_iterate_next( proc->thread_manager, current );
  }

  return forked;
}

/**
 * @fn void task_process_queue_reset(void)
 * @brief Resets process priority queues
 */
void task_process_queue_reset( void ) {
  // min / max queue
  task_priority_queue_t* min_queue = NULL;
  task_priority_queue_t* max_queue = NULL;
  avl_node_t* min = NULL;
  avl_node_t* max = NULL;

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "task_process_queue_reset()\r\n" )
  #endif

  // get min and max priority queue
  min = avl_get_min( process_manager->thread_priority->root );
  max = avl_get_max( process_manager->thread_priority->root );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "min: %p, max: %p\r\n", min, max )
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
    avl_node_t* current_node = avl_find_by_data(
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
    task_priority_queue_t* current = TASK_QUEUE_GET_PRIORITY( current_node );
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
  list_item_t* current = process_manager->process_to_cleanup->first;
  // loop
  while ( current ) {
    // get process from item
    task_process_t* proc = ( task_process_t* )current->data;
    // check for running thread
    avl_node_t* current_thread = avl_iterate_first( proc->thread_manager );
    bool skip = false;
    while ( current_thread ) {
      // get thread
      task_thread_t* thread = TASK_THREAD_GET_BLOCK( current_thread );
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
        DEBUG_OUTPUT( "Skip process with id %d!\r\n", proc->id )
      #endif
      continue;
    }
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Cleanup process with id %d!\r\n", proc->id )
    #endif
    // cache current for removal, cleanup helper destroys process completely
    list_item_t* remove = current;
    // head over to next
    current = current->next;
    // remove list item
    list_remove_item( process_manager->process_to_cleanup, remove );
  }
}

/**
 * @fn bool task_process_prepare_init(task_process_t*)
 * @brief Prepare init process by mapping ramdisk and optional archticture depending and pass it via argv
 *
 * @param proc init process structure
 * @return bool true on success, else false
 */
bool task_process_prepare_init( task_process_t* proc ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "task_process_prepare_init( %p )\r\n", proc )
  #endif

  tar_header_t* ramdisk = tar_lookup_file(
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
    rounded_ramdisk_file_size,
    PHYS_MEMORY_TYPE_NORMAL
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

  char str_ramdisk[ 20 ];
  char str_ramdisk_size[ 20 ];
  sprintf( str_ramdisk, "%#"PRIxPTR"\0", proc_ramdisk_start );
  sprintf( str_ramdisk_size, "%#zx\0", ramdisk_file_size );

  // get thread
  avl_node_t* node = avl_iterate_first( proc->thread_manager );
  if ( ! node ) {
    return false;
  }
  task_thread_t* thread = TASK_THREAD_GET_BLOCK( node );

  // empty env for init
  char* env[] = { NULL, };
  // arch related
  uintptr_t proc_additional_start = task_process_prepare_init_arch( proc );
  if ( proc_additional_start ) {
    char str_additional[ 20 ];
    sprintf( str_additional, "%#"PRIxPTR"\0", proc_additional_start );

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
 * @fn task_process_t* task_process_get_by_id(pid_t)
 * @brief Get task structure by id
 *
 * @param pid
 * @return
 */
task_process_t* task_process_get_by_id( pid_t pid ) {
  // lookup process id tree
  avl_node_t* found = avl_find_by_data(
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
 * @fn void task_process_prepare_kill(void*, task_process_t*)
 * @brief Prepare process kill
 *
 * @param context
 * @param proc
 */
void task_process_prepare_kill( void* context, task_process_t* proc ) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Prepare kill of process %d\r\n", proc->id )
  #endif
  // get first thread
  avl_node_t* current = avl_iterate_first( proc->thread_manager );
  // loop through all threads set appropriate state for kill
  while ( current ) {
    // get thread
    task_thread_t* thread = TASK_THREAD_GET_BLOCK( current );
    // set process state
    thread->state = TASK_THREAD_STATE_KILL;
    // get next thread
    current = avl_iterate_next( proc->thread_manager, current );
  }
  // unregister possible interrupts
  interrupt_unregister_process( proc );
  // push process to clean up list
  list_push_back_data( process_manager->process_to_cleanup, proc );
  // trigger schedule and cleanup
  event_enqueue( EVENT_PROCESS, EVENT_DETERMINE_ORIGIN( context ) );
}

/**
 * @fn int task_process_replace(task_process_t*, uintptr_t, const char**, const char**, void*)
 * @brief Replace current process with elf image
 *
 * @param proc
 * @param elf
 * @param argv
 * @param env
 * @param context
 * @return
 */
int task_process_replace(
  task_process_t* proc,
  uintptr_t elf,
  const char** argv,
  const char** env,
  void* context
) {
  bool replace_current_thread = task_thread_current_thread->process == proc;
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_thread_current_thread->process = %p, proc = %p\r\n",
      task_thread_current_thread->process, proc
    )
  #endif

  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Duplicate argv\r\n" )
  #endif
  // save argv and env if existing
  char** tmp_argv = duplicate( argv );
  if ( ! tmp_argv ) {
    return -ENOMEM;
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Duplicate env\r\n" )
  #endif
  char** tmp_env = duplicate( env );
  if ( ! tmp_env ) {
    free( tmp_argv );
    return -ENOMEM;
  }

  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Preparing to load image\r\n" )
  #endif
  // save image temporary
  size_t image_size = elf_image_size( ( uintptr_t )elf );
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "image_size = %#zx\r\n", image_size )
  #endif
  void* image = malloc( sizeof( char ) * image_size );
  // handle error
  if ( ! image ) {
    free( tmp_argv );
    free( tmp_env );
    return -ENOMEM;
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "image = %p\r\n", image )
  #endif
  memcpy_unsafe_src( image, ( void* )elf, image_size );

  // clear all assigned shared areas
  if ( ! shared_memory_cleanup_process( proc ) ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }

  // destroy virtual context
  if ( ! virt_destroy_context(
    proc->virtual_context,
    true
  ) ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }

  // destroy thread manager
  if ( proc->thread_manager ) {
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "thread manager = %p\r\n", proc->thread_manager )
    #endif
    task_thread_destroy( proc->thread_manager );
  }
  // destroy stack manager
  if ( proc->thread_stack_manager ) {
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "stack manager = %p\r\n", proc->thread_stack_manager )
    #endif
    task_stack_manager_destroy( proc->thread_stack_manager );
  }

  // recreate thread manager and stack manager
  proc->thread_manager = task_thread_init();
  if ( ! proc->thread_manager ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }
  proc->thread_stack_manager = task_stack_manager_create();
  if ( ! proc->thread_stack_manager ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }
  // reset thread id counter
  proc->current_thread_id = 0;

  // load elf image
  uintptr_t init_entry = elf_load( ( uintptr_t )image, proc );
  if ( ! init_entry ) {
    free( tmp_argv );
    free( tmp_env );
    free( image );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }

  // add thread
  task_thread_t* new_current = task_thread_create( init_entry, proc, 0 );
  if ( ! new_current ) {
    free( tmp_argv );
    free( tmp_env );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "new_current->current_context = %p\r\n",
      new_current->current_context
    )
  #endif

  // push arguments and environment
  if ( ! task_thread_push_arguments( new_current, tmp_argv, tmp_env ) ) {
    free( tmp_argv );
    free( tmp_env );
    task_process_prepare_kill( context, proc );
    return -ENOMEM;
  }

  // free temporary stuff
  free( image );
  free( tmp_argv );
  free( tmp_env );

  // replace new current thread
  if ( replace_current_thread ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "Replace current thread!\r\n" )
    #endif
    // replace current thread pointer
    task_thread_current_thread = new_current;
    // switch thread state to active
    task_thread_current_thread->state = TASK_THREAD_STATE_ACTIVE;
  }
  return 0;
}

/**
 * @fn void task_unblock_threads(task_process_t*, task_thread_state_t, task_state_data_t)
 * @brief Helper to unblock threads of process by thread state and data
 *
 * @param proc
 * @param necessary_thread_state
 * @param necessary_thread_data
 */
void task_unblock_threads(
  task_process_t* proc,
  task_thread_state_t necessary_thread_state,
  task_state_data_t necessary_thread_data
) {
  // get first thread
  avl_node_t* current_thread_node = avl_iterate_first( proc->thread_manager );
  // loop until there is no more thread
  while ( current_thread_node ) {
    // get thread
    task_thread_t* possible_thread_to_unblock = TASK_THREAD_GET_BLOCK(
      current_thread_node );
    // try to unblock if blocked
    task_thread_unblock(
      possible_thread_to_unblock,
      necessary_thread_state,
      necessary_thread_data
    );
    // get next thread
    current_thread_node = avl_iterate_next(
      proc->thread_manager,
      current_thread_node );
  }
}
