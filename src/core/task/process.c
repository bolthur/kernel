
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
#include <core/initrd.h>
#include <core/event.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/elf/common.h>
#if defined( PRINT_PROCESS )
  #include <core/debug/debug.h>
#endif
#include <core/task/queue.h>
#include <core/task/process.h>
#include <core/task/thread.h>
#include <core/task/stack.h>

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
static int32_t process_compare_id_callback(
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
  if ( ( size_t )a->data > ( size_t )b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( size_t )b->data > ( size_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
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
    process_compare_id_callback, NULL, NULL );
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

  // register process switch event
  if ( ! event_bind( EVENT_PROCESS, task_process_schedule, true ) ) {
    avl_destroy_tree( process_manager->thread_priority );
    avl_destroy_tree( process_manager->process_id );
    free( process_manager );
    return false;
  }
  // register cleanup
  if ( ! event_bind( EVENT_PROCESS, task_process_cleanup, true ) ) {
    event_unbind( EVENT_PROCESS, task_process_schedule, true );
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
size_t task_process_generate_id( void ) {
  // current pid
  static size_t current = 0;
  // return new pid by simple increment
  return ++current;
}

/**
 * @brief Method to create new process
 *
 * @param entry process entry address
 * @param priority process priority
 * @param parent parent process id
 * @return pointer to created task
 */
task_process_ptr_t task_process_create( uintptr_t entry, size_t priority, size_t parent ) {
  // check manager
  if ( ! process_manager ) {
    return NULL;
  }

  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_process_create( %p, %zu, %zu ) called\r\n",
      ( void* )entry, priority, parent );
  #endif

  // check for valid header
  if ( ! elf_check( entry ) ) {
    // debug output
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "No valid elf header found\r\n" );
    #endif
    // return
    return NULL;
  }

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
  // populate process structure
  process->id = task_process_generate_id();
  process->thread_manager = task_thread_init();
  // handle error
  if ( ! process->thread_manager ) {
    free( process );
    return NULL;
  }
  process->state = TASK_PROCESS_STATE_READY;
  process->priority = priority;
  process->parent = parent;
  process->thread_stack_manager = task_stack_manager_create();
  // handle error
  if ( ! process->thread_stack_manager ) {
    task_thread_destroy( process->thread_manager );
    free( process );
    return NULL;
  }
  // create context only for user processes
  process->virtual_context = virt_create_context( VIRT_CONTEXT_TYPE_USER );
  // handle error
  if ( ! process->virtual_context ) {
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return NULL;
  }

  // load elf executable
  uintptr_t program_entry = elf_load( entry, process );
  // handle error
  if ( 0 == program_entry ) {
    virt_destroy_context( process->virtual_context );
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return NULL;
  }

  // prepare node
  avl_prepare_node( &process->node_id, ( void* )process->id );
  // add process to tree
  if ( ! avl_insert_by_node( process_manager->process_id, &process->node_id ) ) {
    virt_destroy_context( process->virtual_context );
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return NULL;
  }

  // Setup thread with entry
  if ( ! task_thread_create( program_entry, process, priority ) ) {
    avl_remove_by_node( process_manager->process_id, &process->node_id );
    virt_destroy_context( process->virtual_context );
    task_stack_manager_destroy( process->thread_stack_manager );
    task_thread_destroy( process->thread_manager );
    free( process );
    return NULL;
  }
  // return allocated process
  return process;
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
 * @brief Task process cleanup handling
 *
 * @param origin
 * @param context
 *
 * @todo add process cleanup logic
 */
void task_process_cleanup(
  __unused event_origin_t origin,
  __unused void* context
) {
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
  size_t rounded_ramdisk_file_size = ramdisk_file_size;
  ROUND_UP_TO_FULL_PAGE( rounded_ramdisk_file_size )
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

  // arch related
  uintptr_t proc_additional_start = task_process_prepare_init_arch( proc );
  if ( proc_additional_start ) {
    char str_additional[ 20 ];
    sprintf(
      str_additional, "%#0*"PRIxPTR"\0", addr_size, proc_additional_start
    );

    assert( task_thread_push_arguments(
      proc, "./init", str_ramdisk, str_ramdisk_size, str_additional, NULL
    ) );
  } else {
    assert( task_thread_push_arguments(
      proc, "./init", str_ramdisk, str_ramdisk_size, NULL
    ) );
  }

  return true;
}
