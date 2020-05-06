
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <core/arch.h>
#include <core/elf/common.h>
#include <core/tty.h>
#include <core/interrupt.h>
#include <core/timer.h>
#include <core/platform.h>
#include <core/debug/debug.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/mm/heap.h>
#include <core/event.h>
#include <core/task/process.h>
#include <core/syscall.h>

#if defined( REMOTE_DEBUG )
  #include <core/serial.h>
  #include <core/debug/gdb.h>
#endif

#include <tar.h>
#include <core/panic.h>
#include <core/initrd.h>

// disable missing prototype temporarily
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"

/**
 * @brief Kernel main function
 *
 * @todo remove initrd test code later
 */
void kernel_main( void ) {
  // Setup early heap for malloc / free support
  DEBUG_OUTPUT( "[bolthur/kernel -> heap] early heap initialize ...\r\n" );
  heap_init( HEAP_INIT_EARLY );

  // setup tty for output
  DEBUG_OUTPUT( "[bolthur/kernel -> heap] tty initialize ...\r\n" );
  tty_init();

  // Some initial output :)
  DEBUG_OUTPUT(
    "\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n",
    " _           _ _   _                      __  _                        _ ",
    "| |         | | | | |                    / / | |                      | |",
    "| |__   ___ | | |_| |__  _   _ _ __     / /  | | _____ _ __ _ __   ___| |",
    "| '_ \\ / _ \\| | __| '_ \\| | | | '__|   / /   | |/ / _ \\ '__| '_ \\ / _ \\ |",
    "| |_) | (_) | | |_| | | | |_| | |     / /    |   <  __/ |  | | | |  __/ |",
    "|_.__/ \\___/|_|\\__|_| |_|\\__,_|_|    /_/     |_|\\_\\___|_|  |_| |_|\\___|_|"
  );

  // Setup event system
  DEBUG_OUTPUT( "[bolthur/kernel -> event] initialize ...\r\n" );
  event_init();

  // Setup interrupt
  DEBUG_OUTPUT( "[bolthur/kernel -> interrupt] initialize ...\r\n" );
  interrupt_init();

  // remote gdb debugging
  #if defined( REMOTE_DEBUG )
    // initialize serial
    DEBUG_OUTPUT( "[bolthur/kernel -> serial ] serial init ...\r\n" );
    serial_init();

    // register serial interrupt ( irg/fiq )
    DEBUG_OUTPUT( "[bolthur/kernel -> serial ] register interrupt ...\r\n" );
    serial_register_interrupt();

    // Setup gdb stub
    DEBUG_OUTPUT( "[bolthur/kernel -> debug -> gdb] initialize ...\r\n" );
    debug_gdb_init();
  #endif

  // Setup arch related parts
  DEBUG_OUTPUT( "[bolthur/kernel -> arch] initialize ...\r\n" );
  arch_init();

  // Setup platform related parts
  DEBUG_OUTPUT( "[bolthur/kernel -> platform] initialize ...\r\n" );
  platform_init();

  // Setup initrd parts
  DEBUG_OUTPUT( "[bolthur/kernel -> initrd] initialize ...\r\n" );
  initrd_init();

  // Setup physical memory management
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> physical] initialize ...\r\n" );
  phys_init();

  // Setup virtual memory management
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> virtual] initialize ...\r\n" );
  virt_init();

  // print size
  if ( initrd_exist() ) {
    uintptr_t initrd = initrd_get_start_address();
    DEBUG_OUTPUT( "initrd = %p\r\n", ( void* )initrd );
    DEBUG_OUTPUT( "initrd = %p\r\n", ( void* )initrd_get_end_address() );
    DEBUG_OUTPUT( "size = %zo\r\n", initrd_get_size() );
    DEBUG_OUTPUT( "size = %zu\r\n", initrd_get_size() );

    // set iterator
    tar_header_ptr_t iter = ( tar_header_ptr_t )initrd;

    // loop through tar
    while ( ! tar_end_reached( iter ) ) {
      // debug output
      DEBUG_OUTPUT( "%p: initrd file name: %s\r\n",
        ( void* )iter, iter->file_name );

      // next
      iter = tar_next( iter );
    }
  }

  // Setup heap
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> heap] initialize ...\r\n" );
  heap_init( HEAP_INIT_NORMAL );

  // Setup multitasking
  DEBUG_OUTPUT( "[bolthur/kernel -> process] initialize ...\r\n" );
  task_process_init();

  // setup syscalls
  DEBUG_OUTPUT( "[bolthur/kernel -> syscall] initialize ...\r\n" );;
  syscall_init();

  // FIXME: Create init process from initialramdisk and pass initrd to init process
  // create processes for elf files
  if ( initrd_exist() ) {
    // get iterator
    tar_header_ptr_t iter = ( tar_header_ptr_t )initrd_get_start_address();

    // loop
    while ( ! tar_end_reached( iter ) ) {
      // get file
      DEBUG_OUTPUT( "Current file %s\r\n", iter->file_name );
      uintptr_t file = ( uintptr_t )tar_file( iter );

      // skip non elf files
      if ( elf_check( file ) ) {
        // create process
        uint64_t file_size = tar_size( iter );
        DEBUG_OUTPUT( "Create process for file %s\r\n", iter->file_name );
        DEBUG_OUTPUT( "File size: %#llx\r\n", file_size );

        task_process_create( file, 0 );
        task_process_create( file, 0 );
        task_process_create( file, 0 );
      }

      // next task
      iter = tar_next( iter );
    }
  }

  // Setup timer
  DEBUG_OUTPUT( "[bolthur/kernel -> timer] initialize ...\r\n" );
  timer_init();

  // kickstart multitasking
  task_process_start();
}

// enable again
#pragma GCC diagnostic pop
