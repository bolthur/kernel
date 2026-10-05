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

#include <stddef.h>
#include "../../../../lib/stdlib.h"
#include "../../../../lib/string.h"
#include "../../stack.h"
#include "../../../../mm/phys.h"
#include "../../../../mm/virt.h"
#include "../../../../syscall.h"
#if defined( PRINT_PROCESS )
  #include "../../../../lib/inttypes.h"
  #include "../../../../debug/debug.h"
#endif
#include "../../../../task/queue.h"
#include "../../../../task/process.h"
#include "../../../../task/thread.h"
#include "../../../../task/stack.h"
#include "../cpu.h"

// simple macro to encapsulate push to stack
#define STACK_PUSH( sp, user_sp, type, val ) \
  { \
    user_sp -= sizeof( type ); \
    sp -= sizeof( type ); \
    *(type*)sp = val; \
  }

/**
 * @fn task_thread_t task_thread_create*(uintptr_t, task_process_t*, size_t)
 * @brief Method to create thread structure
 *
 * @param entry entry point of the thread
 * @param process thread process
 * @param nice_level thread nice_level
 * @return task_thread_t* pointer to thread structure
 */
task_thread_t* task_thread_create(
  uintptr_t entry,
  task_process_t* process,
  size_t nice_level
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "task_thread_create( %#"PRIxPTR", %p, %zu ) called\r\n",
      entry,
      process,
      priority
    )
  #endif

  // create stack
  uint64_t* stack_physical = task_stack_manager_allocate_stack( STACK_SIZE );
  if ( ! stack_physical) {
    return nullptr;
  }

  // get next stack address for user area
  uintptr_t stack_virtual = task_stack_manager_next(
    process->thread_stack_manager
  );
  // handle error
  if ( 0 == stack_virtual ) {
    task_stack_manager_cleanup_stack( stack_physical, STACK_SIZE );
    return nullptr;
  }
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "stack_virtual = %#"PRIxPTR"\r\n", stack_virtual )
  #endif

  // create thread structure
  task_thread_t* thread = malloc( sizeof( *thread ) );
  // check
  if ( ! thread ) {
    task_stack_manager_cleanup_stack( stack_physical, STACK_SIZE );
    return nullptr;
  }
  // prepare
  memset( thread, 0, sizeof( task_thread_t ) );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "Reserved space for thread structure at %p\r\n", thread )
  #endif

  // create context
  thread->current_context = malloc( sizeof( cpu_register_context_t ) );
  // handle error
  if ( ! thread->current_context ) {
    task_stack_manager_cleanup_stack( stack_physical, STACK_SIZE );
    free( thread );
    return nullptr;
  }

  // cache locally
  auto current_context = ( cpu_register_context_t* )thread->current_context;
  // prepare area
  memset( current_context, 0, sizeof( cpu_register_context_t ) );
  // set content
  current_context->reg.pc = ( uint32_t )entry & ~1U;
  // only user mode threads are possible
  current_context->reg.spsr = CPSR_MODE_USER;
  // add arm thumb mode to spsr if necessary
  if ( ( uint32_t )entry & 0x1 ) {
    // add thumb mode to spsr
    current_context->reg.spsr |= CPSR_THUMB;
  }
  // set stack pointer
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT(
      "%#"PRIxPTR" / %#"PRIxPTR"\r\n",
      ( uintptr_t )( stack_virtual + STACK_SIZE - sizeof( int ) ),
      ( uintptr_t )( stack_virtual + STACK_SIZE - alignof( max_align_t ) )
    )
    DUMP_REGISTER(current_context);
  #endif
  current_context->reg.sp = stack_virtual - alignof( max_align_t );
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "%d\r\n", alignof( max_align_t ) )
    DUMP_REGISTER(current_context);
  #endif
  // push back current value of fpu
  #if defined( ARM_CPU_HAS_NEON )
    __asm__ __volatile__(
      "vmrs %0, fpscr"
      : "=r" ( current_context->reg.fpscr )
      : : "cc", "memory"
    );
  #endif
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( current_context )
  #endif

  // map stack temporary
  uintptr_t tmp_virtual_user = virt_map_temporary_range( stack_physical, STACK_SIZE );
  // handle error
  if ( 0 == tmp_virtual_user ) {
    task_stack_manager_cleanup_stack( stack_physical, STACK_SIZE );
    free( thread->current_context );
    free( thread );
    return nullptr;
  }
  // prepare stack
  memset( ( void* )tmp_virtual_user, 0, STACK_SIZE );
  // unmap again
  virt_unmap_temporary( tmp_virtual_user, STACK_SIZE );
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "stack_virtual = %#"PRIxPTR"\r\n", stack_virtual )
    DEBUG_OUTPUT( "stack_physical = %#"PRIx64"\r\n", stack_physical )
  #endif
  // create node for stack address management tree
  if ( ! task_stack_manager_add(
    stack_virtual,
    process->thread_stack_manager
  ) ) {
    task_stack_manager_cleanup_stack( stack_physical, STACK_SIZE );
    free( thread->current_context );
    free( thread );
    return nullptr;
  }
  uintptr_t virtual = stack_virtual - PAGE_SIZE;
  size_t physical_index = ( STACK_SIZE / PAGE_SIZE ) - 1;
  for(
    uintptr_t stack_current = 0;
    stack_current < STACK_SIZE;
    stack_current += PAGE_SIZE, virtual -= PAGE_SIZE
  ) {
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "virtual = %#"PRIxPTR"\r\n", virtual )
      DEBUG_OUTPUT( "physical = %#"PRIx64"\r\n", physical )
    #endif
    // map stack
    if ( ! virt_map_address(
      process->virtual_context,
      virtual,
      stack_physical[ physical_index-- ],
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) ) {
      task_stack_manager_remove( stack_virtual, process->thread_stack_manager );
      task_stack_manager_cleanup_stack( stack_physical, STACK_SIZE );
      free( thread->current_context );
      free( thread );
      return nullptr;
    }
  }

  // populate thread data
  task_thread_set_state( thread, TASK_THREAD_STATE_READY );
  thread->entry = entry;
  thread->id = task_thread_generate_id();
  thread->process = process;
  thread->stack_physical = stack_physical;
  thread->stack_virtual = stack_virtual;
  thread->stack_size = STACK_SIZE;
  thread->nice_level = nice_level;
  thread->vruntime = 0;
  thread->weight = task_thread_priority_weight[ thread->nice_level + 20 ];
  // push back into free list
  if ( ! list_push_back_data( process->thread_list, thread ) ) {
    task_stack_manager_remove( stack_virtual, process->thread_stack_manager );
    virt_unmap_address( process->virtual_context, stack_virtual, true ); /// FIXME: UNMAP WHOLE STACK COMPLETELY
    free( thread->current_context );
    free( thread );
    return nullptr;
  }
  // enqueue thread
  task_queue_enqueue( thread );
  // return created thread
  return thread;
}

/**
 * @fn task_thread_t* task_thread_fork(task_process_t*, task_thread_t*)
 * @brief Create a copy of a thread of a process
 * @param forked_process process where thread shall be pushed into
 * @param thread_to_fork thread to be forked
 * @return new thread structure or nullptr
 */
task_thread_t* task_thread_fork(
  task_process_t* forked_process,
  const task_thread_t* thread_to_fork
) {
  // reserve space for new management structure
  task_thread_t* thread = malloc( sizeof( *thread ) );
  // handle error
  if ( ! thread ) {
    return nullptr;
  }
  // erase memory
  memset( thread, 0, sizeof( task_thread_t ) );

  // reserve space for context
  thread->current_context = malloc( sizeof( cpu_register_context_t ) );
  // handle error
  if ( ! thread->current_context ) {
    free( thread );
    return nullptr;
  }
  // erase memory
  memset( thread->current_context, 0, sizeof( cpu_register_context_t ) );

  // populate data
  thread->process = forked_process;
  thread->id = task_thread_generate_id();
  thread->stack_virtual = thread_to_fork->stack_virtual;
  thread->entry = thread_to_fork->entry;
  thread->handling_interrupt = thread_to_fork->handling_interrupt;
  uint64_t* physical = calloc( thread_to_fork->stack_size / PAGE_SIZE, sizeof( uint64_t ) );
  if ( ! physical ) {
    free( thread->current_context );
    free( thread );
    return nullptr;
  }
  for ( size_t i = 0; i < thread->stack_size / PAGE_SIZE; i++ ) {
    physical[ i ] = virt_get_mapped_address_in_context(
      thread->process->virtual_context,
      thread->stack_virtual - i * PAGE_SIZE
    );
  }
  thread->stack_physical = physical;
  // copy over weight, vruntime and nice level
  thread->weight = thread_to_fork->weight;
  thread->vruntime = thread_to_fork->vruntime;
  thread->nice_level = thread_to_fork->nice_level;

  thread->stack_size = thread_to_fork->stack_size;
  task_thread_set_state( thread, TASK_THREAD_STATE_READY );
  // copy register context data
  memcpy(
    thread->current_context,
    thread_to_fork->current_context,
    sizeof( cpu_register_context_t )
  );
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( thread->current_context )
  #endif
  // overwrite return on forked thread with 0
  syscall_populate_success( thread->current_context, 0 );
  // debug output
  #if defined( PRINT_PROCESS )
    DUMP_REGISTER( thread->current_context )
  #endif

  // create node for stack address management tree
  if ( ! task_stack_manager_add(
    thread->stack_virtual,
    thread->process->thread_stack_manager
  ) ) {
    free( thread->stack_physical );
    free( thread->current_context );
    free( thread );
    return nullptr;
  }

  // push back thread into free list
  if ( ! list_push_back_data( thread->process->thread_list, thread ) ) {
    task_stack_manager_remove(
      thread->stack_virtual,
      thread->process->thread_stack_manager
    );
    free( thread->stack_physical );
    free( thread->current_context );
    free( thread );
    return nullptr;
  }
  task_queue_enqueue( thread );
  return thread;
}

/**
 * @fn bool task_thread_push_arguments(task_thread_t*, char**)
 * @brief Small helper to push argument list for thread to stack
 *
 * @param thread
 * @param argument
 * @param environment
 * @return
 */
bool task_thread_push_arguments(
  const task_thread_t* thread,
  char** argument,
  char** environment
) {
  int argv_count = 0;
  int env_count = 0;
  // determine count of argv
  while( argument && argument[ argv_count ] ) {
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "%s\r\n", argument[ argv_count ] )
    #endif
    // increment entry count
    argv_count++;
  }
  // determine count of env
  while( environment && environment[ env_count ] ) {
    #if defined( PRINT_PROCESS )
      DEBUG_OUTPUT( "%s\r\n", environment[ env_count ] )
    #endif
    // increment count
    env_count++;
  }
  // allocate pointer structure for env
  uintptr_t* env_ptr = nullptr;
  if ( 0 < env_count ) {
    env_ptr = malloc( ( size_t )env_count * sizeof( uintptr_t ) );
    if ( ! env_ptr ) {
      return false;
    }
    memset( env_ptr, 0, ( size_t )env_count * sizeof( uintptr_t ) );
  }
  // allocate pointer structure for env
  uintptr_t* argv_ptr = nullptr;
  if ( 0 < argv_count ) {
    argv_ptr = malloc( ( size_t )argv_count * sizeof( uintptr_t ) );
    if ( ! argv_ptr ) {
      free( env_ptr );
      return false;
    }
    memset( argv_ptr, 0, ( size_t )argv_count * sizeof( uintptr_t ) );
  }
  // map stack temporarily
  const uintptr_t stack_tmp = virt_map_temporary_range( thread->stack_physical, thread->stack_size );
  if ( !stack_tmp ) {
    free( env_ptr );
    free( argv_ptr );
    return false;
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "stack_tmp = %#"PRIxPTR", thread->stack_virtual = %#"PRIxPTR"\r\n", stack_tmp, thread->stack_virtual )
  #endif
  // get top stack of temporary and user
  uintptr_t rsp = stack_tmp + thread->stack_size - alignof( max_align_t );
  uintptr_t user_rsp = thread->stack_virtual - alignof( max_align_t );
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
  #endif
  // push env to stack
  if ( env_count ) {
    // iterate from last to first
    for (int i = env_count - 1; i >= 0; i-- ) {
      // get env length
      const size_t len = strlen( environment[ i ] ) + 1;
      // adjust rsp and user rsp
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
      #endif
      rsp -= len;
      user_rsp -= len;
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
      #endif
      // copy over data
      memcpy( ( void* )rsp, environment[ i ], len );
      // store user pointer in array
      env_ptr[ i ] = user_rsp;
    }
  }
  // push argv to stack
  if ( argv_count ) {
    // iterate from last to first
    for (int i = argv_count - 1; i >= 0; i-- ) {
      // get argument length
      const size_t len = strlen( argument[ i ] ) + 1;
      // adjust rsp and user rsp
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
      #endif
      rsp -= len;
      user_rsp -= len;
      #if defined( PRINT_PROCESS )
        DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
      #endif
      // copy over data
      memcpy( ( void* )rsp, argument[ i ], len );
      // store user pointer in array
      argv_ptr[ i ] = user_rsp;
    }
  }
  // determine total pushes, which is argv count + null termination as well as
  // environment count + null termination
  const size_t total_pushes = ( size_t )argv_count + 1 + ( size_t )env_count + 1;
  const size_t push_space = total_pushes * sizeof( void* );
  // align stack properly before pushing argc, argv and env
  const uintptr_t target_rsp = rsp - push_space;
  const uintptr_t target_user_rsp = user_rsp - push_space;
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "target_rsp = %#"PRIxPTR", target_user_rsp = %#"PRIxPTR"\r\n", target_rsp, target_user_rsp )
  #endif
  const uintptr_t aligned_rsp = rsp % alignof( max_align_t );
  const uintptr_t aligned_user_rsp = user_rsp % alignof( max_align_t );
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "aligned_rsp = %#"PRIxPTR", aligned_user_rsp = %#"PRIxPTR"\r\n", aligned_rsp, aligned_user_rsp )
    DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
  #endif
  rsp = target_rsp - aligned_rsp;
  user_rsp = target_user_rsp - aligned_user_rsp;
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
  #endif
  // push environment to stack
  STACK_PUSH( rsp, user_rsp, uintptr_t, 0 );
  for ( int i = env_count - 1; i >= 0; i-- ) {
    STACK_PUSH( rsp, user_rsp, uintptr_t, env_ptr[ i ] );
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
  #endif
  // cache env start
  const uintptr_t env_start = user_rsp;
  // push argv to stack
  STACK_PUSH( rsp, user_rsp, uintptr_t, 0 );
  for ( int i = argv_count - 1; i >= 0; i-- ) {
    STACK_PUSH( rsp, user_rsp, uintptr_t, argv_ptr[ i ] );
  }
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "rsp = %#"PRIxPTR", user_rsp = %#"PRIxPTR"\r\n", rsp, user_rsp )
  #endif
  // push argv start
  const uintptr_t argv_start = user_rsp;
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "env_start = %#"PRIxPTR", argv_start = %#"PRIxPTR"\r\n", env_start, argv_start )
  #endif
  // populate r0 - r2 ( argv, argc and env )
  auto const cpu = ( cpu_register_context_t* )thread->current_context;
  cpu->reg.r0 = ( uint32_t )argv_count;
  cpu->reg.r1 = argv_start;
  cpu->reg.r2 = env_start;
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "cpu->reg.sp = %#"PRIx32"\r\n", cpu->reg.sp )
    DUMP_REGISTER( cpu )
  #endif
  // set adjusted stack pointer
  cpu->reg.sp = user_rsp;
  // unmap again
  virt_unmap_temporary( stack_tmp, STACK_SIZE );
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "cpu->reg.sp = %#"PRIx32"\r\n", cpu->reg.sp )
    DUMP_REGISTER( cpu )
  #endif
  // free up env ptr and argv ptr again
  free( env_ptr );
  free( argv_ptr );
  // return success
  return true;
}
