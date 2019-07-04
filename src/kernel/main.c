
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
#include <kernel/event.h>
#include <kernel/vfs.h>

/**
 * @brief Kernel main function
 */
void kernel_main() {
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

  // Setup physical memory management
  printf( "[bolthur/kernel -> memory -> physical] initialize ...\r\n" );
  phys_init();

  // Setup virtual memory management
  printf( "[bolthur/kernel -> memory -> virtual] initialize ...\r\n" );
  virt_init();

  // Setup heap
  printf( "[bolthur/kernel -> memory -> heap] initialize ...\r\n" );
  heap_init();

  // some heap testing code
  void *a;
  for( int i = 0; i < 40; i++ ) {
    a = malloc( 4 );
  }
  a = malloc( 12 );
  void *b = malloc( 4 );
  void *c = malloc( 8 );
  void *d = malloc( 10 );
  void *e = malloc( 4 );
  printf( "\r\n\r\nFREE UP 0x%08X\r\n", b );
  free( b );
  b = malloc( 4 );
  printf( "\r\n\r\nFREE UP 0x%08X\r\n", b );
  free( b );
  printf( "FREE UP 0x%08X\r\n", c );
  free( c );
  printf( "FREE UP 0x%08X\r\n", a );
  free( a );
  printf( "FREE UP 0x%08X\r\n", d );
  free( d );
  printf( "FREE UP 0x%08X\r\n", e );
  free( e );
  b = malloc( 0x10000 );
  printf( "\r\n\r\nFREE UP 0x%08X\r\n", b );
  free( b );

  // Setup event system
  printf( "[bolthur/kernel -> event] initialize ...\r\n" );
  event_init();

  // Setup vfs
  printf( "[bolthur/kernel -> vfs] initialize ...\r\n" );
  vfs_init();

  // Setup timer
  printf( "[bolthur/kernel -> timer] initialize ...\r\n" );
  timer_init();

  // Setup irq
  printf( "[bolthur/kernel -> irq] enable ...\r\n" );
  irq_enable();

  while ( 1 ) {}
}
