
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
#include <kernel/debug.h>
#include <kernel/serial.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>
#include <kernel/mm/placement.h>
#include <kernel/event.h>

#include <endian.h>
#include <tar.h>
#include <kernel/panic.h>
#include <kernel/initrd.h>

/**
 * @brief Kernel main function
 *
 * @todo remove initrd test code later
 */
void kernel_main( void ) {
  // Initialize serial for debugging if enabled
  #if defined( DEBUG )
    serial_init();
  #endif

  // enable tty for output
  tty_init();

  // Some initial output :)
  printf(
    "\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n",
    " _           _ _   _                      __  _                        _ ",
    "| |         | | | | |                    / / | |                      | |",
    "| |__   ___ | | |_| |__  _   _ _ __     / /  | | _____ _ __ _ __   ___| |",
    "| '_ \\ / _ \\| | __| '_ \\| | | | '__|   / /   | |/ / _ \\ '__| '_ \\ / _ \\ |",
    "| |_) | (_) | | |_| | | | |_| | |     / /    |   <  __/ |  | | | |  __/ |",
    "|_.__/ \\___/|_|\\__|_| |_|\\__,_|_|    /_/     |_|\\_\\___|_|  |_| |_|\\___|_|"
  );

  // Setup arch related parts
  printf( "[bolthur/kernel -> arch] initialize ...\r\n" );
  arch_init();

  // Setup platform related parts
  printf( "[bolthur/kernel -> platform] initialize ...\r\n" );
  platform_init();

  // Setup irq
  printf( "[bolthur/kernel -> irq] initialize ...\r\n" );
  irq_init();

  // Setup initrd parts
  printf( "[bolthur/kernel -> initrd] initialize ...\r\n" );
  initrd_init();

  // print size
  if ( initrd_exist() ) {
    uintptr_t initrd = initrd_get_start_address();
    printf( "initrd = 0x%08x\r\n", initrd );
    printf( "initrd = 0x%08x\r\n", initrd_get_end_address() );
    printf( "size = %o\r\n", initrd_get_size() );
    printf( "size = %d\r\n", initrd_get_size() );
    // set iterator
    tar_header_ptr_t iter = ( tar_header_ptr_t )initrd;
    // loop through tar
    while ( ! tar_end_reached( iter ) ) {
      printf( "0x%lx: initrd file name: %s\r\n", (uintptr_t)iter, iter->file_name );
      iter = tar_next( iter );
    }
  }

  // Setup physical memory management
  printf( "[bolthur/kernel -> memory -> physical] initialize ...\r\n" );
  phys_init();

  // Setup virtual memory management
  printf( "[bolthur/kernel -> memory -> virtual] initialize ...\r\n" );
  virt_init();

  // print size
  if ( initrd_exist() ) {
    uintptr_t initrd = initrd_get_start_address();
    printf( "initrd = 0x%08x\r\n", initrd );
    printf( "initrd = 0x%08x\r\n", initrd_get_end_address() );
    printf( "size = %o\r\n", initrd_get_size() );
    printf( "size = %d\r\n", initrd_get_size() );
    // set iterator
    tar_header_ptr_t iter = ( tar_header_ptr_t )initrd;
    // loop through tar
    while ( ! tar_end_reached( iter ) ) {
      printf( "0x%lx: initrd file name: %s\r\n", (uintptr_t)iter, iter->file_name );
      iter = tar_next( iter );
    }
  }

  // Setup heap
  printf( "[bolthur/kernel -> memory -> heap] initialize ...\r\n" );
  heap_init();

  // Setup event system
  printf( "[bolthur/kernel -> event] initialize ...\r\n" );
  event_init();

  // Setup timer
  printf( "[bolthur/kernel -> timer] initialize ...\r\n" );
  timer_init();

  // Setup irq
  printf( "[bolthur/kernel -> irq] enable ...\r\n" );
  irq_enable();

  while ( 1 ) {}
}
