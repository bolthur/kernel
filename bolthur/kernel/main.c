/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <stdint.h>
#include <stdnoreturn.h>
#include "lib/string.h"
#include "lib/stdio.h"
#include "lib/tar.h"
#include "arch.h"
#include "elf.h"
#include "tty.h"
#include "interrupt.h"
#include "timer.h"
#include "debug/debug.h"
#include "mm/phys.h"
#include "mm/virt.h"
#include "mm/heap.h"
#include "mm/shared.h"
#include "event.h"
#include "task/process.h"
#include "syscall.h"
#if defined( REMOTE_DEBUG )
  #include "serial.h"
  #include "debug/gdb.h"
#endif
#include "initrd.h"

// prototype declaration to get rid of a warning
void kernel_main( void );

/**
 * @fn void kernel_main(void)
 * @brief Kernel main entry
 */
noreturn void kernel_main( void ) {
  // Setup early heap for malloc / free support
  DEBUG_OUTPUT( "[bolthur/kernel -> heap] early heap initialize ...\r\n" )
  heap_init( HEAP_INIT_EARLY );

  // setup tty for output
  DEBUG_OUTPUT( "[bolthur/kernel ->tty] initialize ...\r\n" )
  tty_init();

  // Some initial output :)
  DEBUG_OUTPUT(
    "bolthur/kernel " PACKAGE_VERSION " ( commit #" PACKAGE_REVISION
    " ) compiled with " PACKAGE_COMPILER " for target " PACKAGE_ARCHITECTURE
    " \r\n"
  )

  // Setup event system
  DEBUG_OUTPUT( "[bolthur/kernel -> event] initialize ...\r\n" )
  assert( event_init() )

  // Setup interrupt
  DEBUG_OUTPUT( "[bolthur/kernel -> interrupt] initialize ...\r\n" )
  interrupt_init();

  // remote gdb debugging
  #if defined( REMOTE_DEBUG )
    // initialize serial
    DEBUG_OUTPUT( "[bolthur/kernel -> serial ] serial init ...\r\n" )
    serial_init();

    // register serial interrupt ( irg/fiq )
    DEBUG_OUTPUT( "[bolthur/kernel -> serial ] register interrupt ...\r\n" )
    assert( serial_register_interrupt() )

    // Setup gdb stub
    DEBUG_OUTPUT( "[bolthur/kernel -> debug -> gdb] initialize ...\r\n" )
    debug_gdb_init();
  #endif

  // Setup arch related parts
  DEBUG_OUTPUT( "[bolthur/kernel -> arch] initialize ...\r\n" )
  arch_init();

  // Setup physical memory management
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> physical] initialize ...\r\n" )
  phys_init();

  // Setup virtual memory management
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> virtual] initialize ...\r\n" )
  virt_init();

  // Setup heap
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> heap] initialize ...\r\n" )
  heap_init( HEAP_INIT_NORMAL );

  // Setup shared
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> shared] initialize ...\r\n" )
  assert( shared_memory_init() )

  // Setup multitasking
  DEBUG_OUTPUT( "[bolthur/kernel -> process] initialize ...\r\n" )
  assert( task_process_init() )

  // Setup system calls
  DEBUG_OUTPUT( "[bolthur/kernel -> syscall] initialize ...\r\n" )
  assert( syscall_init() )

  // assert initrd necessary now
  assert( initrd_exist() )
  // Find init process
  tar_header_ptr_t boot = tar_lookup_file( initrd_get_start_address(), "boot" );
  assert( boot )
  // Get file address and size
  uintptr_t elf_file = ( uintptr_t )tar_file( boot );

  // Create process
  DEBUG_OUTPUT( "[bolthur/kernel -> process -> init] create ...\r\n" )
  task_process_ptr_t proc = task_process_create( 0, 0 );
  assert( proc )
  // load flat image
  uintptr_t init_entry = elf_load( elf_file, proc );
  assert( init_entry )
  // add thread
  assert( task_thread_create( init_entry, proc, 0 ) );
  // further init process preparation
  DEBUG_OUTPUT( "[bolthur/kernel -> process -> init] prepare ...\r\n" )
  assert( task_process_prepare_init( proc ) )

  // Setup timer
  DEBUG_OUTPUT( "[bolthur/kernel -> timer] initialize ...\r\n" )
  timer_init();

  // Start multitasking in case that init has been created
  DEBUG_OUTPUT( "[bolthur/kernel -> task] start multitasking ...\r\n" )
  task_process_start();

  // Endless loop
  for(;;);
}
