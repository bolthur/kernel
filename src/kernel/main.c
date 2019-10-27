
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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
#include <kernel/arch.h>
#include <kernel/tty.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/platform.h>
#include <kernel/debug/debug.h>
#include <kernel/serial.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>
#include <kernel/mm/placement.h>
#include <kernel/event.h>
#include <kernel/task/process.h>

#include <endian.h>
#include <tar.h>
#include <kernel/panic.h>
#include <kernel/initrd.h>

static void process1( void ) {
  printf( "A" );
}

static void process2( void ) {
  printf( "B" );
}

static void process3( void ) {
  printf( "C" );
}

/**
 * @brief Kernel main function
 *
 * @todo remove initrd test code later
 * @todo initialize serial when remote debugging is enabled
 */
void kernel_main( void ) {
  // enable tty for output
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

  // Setup arch related parts
  DEBUG_OUTPUT( "[bolthur/kernel -> arch] initialize ...\r\n" );
  arch_init();

  // Setup platform related parts
  DEBUG_OUTPUT( "[bolthur/kernel -> platform] initialize ...\r\n" );
  platform_init();

  // Setup irq
  DEBUG_OUTPUT( "[bolthur/kernel -> irq] initialize ...\r\n" );
  irq_init();

  // Setup initrd parts
  DEBUG_OUTPUT( "[bolthur/kernel -> initrd] initialize ...\r\n" );
  initrd_init();

  // print size
  if ( initrd_exist() ) {
    uintptr_t initrd = initrd_get_start_address();
    DEBUG_OUTPUT( "initrd = 0x%08x\r\n", initrd );
    DEBUG_OUTPUT( "initrd = 0x%08x\r\n", initrd_get_end_address() );
    DEBUG_OUTPUT( "size = %o\r\n", initrd_get_size() );
    DEBUG_OUTPUT( "size = %d\r\n", initrd_get_size() );
    // set iterator
    tar_header_ptr_t iter = ( tar_header_ptr_t )initrd;
    // loop through tar
    while ( ! tar_end_reached( iter ) ) {
      DEBUG_OUTPUT( "0x%lx: initrd file name: %s\r\n", (uintptr_t)iter, iter->file_name );
      iter = tar_next( iter );
    }
  }

  // Setup physical memory management
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> physical] initialize ...\r\n" );
  phys_init();

  // Setup virtual memory management
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> virtual] initialize ...\r\n" );
  virt_init();

  // print size
  if ( initrd_exist() ) {
    uintptr_t initrd = initrd_get_start_address();
    DEBUG_OUTPUT( "initrd = 0x%08x\r\n", initrd );
    DEBUG_OUTPUT( "initrd = 0x%08x\r\n", initrd_get_end_address() );
    DEBUG_OUTPUT( "size = %o\r\n", initrd_get_size() );
    DEBUG_OUTPUT( "size = %d\r\n", initrd_get_size() );
    // set iterator
    tar_header_ptr_t iter = ( tar_header_ptr_t )initrd;
    // loop through tar
    while ( ! tar_end_reached( iter ) ) {
      DEBUG_OUTPUT( "0x%lx: initrd file name: %s\r\n", (uintptr_t)iter, iter->file_name );
      iter = tar_next( iter );
    }
  }

  // Setup heap
  DEBUG_OUTPUT( "[bolthur/kernel -> memory -> heap] initialize ...\r\n" );
  heap_init();

  // Setup event system
  DEBUG_OUTPUT( "[bolthur/kernel -> event] initialize ...\r\n" );
  event_init();

  // Setup timer
  DEBUG_OUTPUT( "[bolthur/kernel -> timer] initialize ...\r\n" );
  timer_init();

  // Setup multitasking
  DEBUG_OUTPUT( "[bolthur/kernel -> process] initialize ...\r\n" );
  task_process_init();

  // create some dummy processes
  task_process_create( ( uintptr_t )process1, TASK_PROCESS_TYPE_KERNEL );
  task_process_create( ( uintptr_t )process2, TASK_PROCESS_TYPE_KERNEL );
  task_process_create( ( uintptr_t )process3, TASK_PROCESS_TYPE_USER );

  // Setup irq
  DEBUG_OUTPUT( "[bolthur/kernel -> irq] enable ...\r\n" );
  irq_enable();

  while ( 1 ) {}
}
