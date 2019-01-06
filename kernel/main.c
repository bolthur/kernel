
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>

#include <tty.h>
#include <event.h>
#include <irq.h>
#include <timer.h>
#include <platform.h>
#include <debug.h>
#include <serial.h>

/**
 * @brief Kernel main function
 */
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
  // enable tty for output
  tty_init();

  // Setup platform related parts
  printf( "[bolthur/kernel -> platform] initialize ... " );
  platform_init();
  printf( "done!\r\n" );

  // Setup irq
  printf( "[bolthur/kernel -> irq] initialize ... " );
  irq_init();
  printf( "done!\r\n" );

  // Setup memory management
  printf( "[bolthur/kernel -> memory] initialize ... [" );
  // FIXME: Setup physical memory management
  printf( " physical, " );
  // FIXME: Setup virtual memory management
  printf( " virtual, " );
  // FIXME: Setup heap
  printf( " heap ] " );
  printf( "done!\r\n" );

  // FIXME: Setup event system
  printf( "[bolthur/kernel -> event] initialize ... " );
  event_init();
  printf( "done!\r\n" );

  // Setup debug if enabled
  #if defined( DEBUG )
    printf( "[bolthur/kernel -> debug] initialize ... " );
    debug_init();
    printf( "done!\r\n" );
  #endif

  // Setup timer
  printf( "[bolthur/kernel -> timer] initialize ... " );
  timer_init();
  printf( "done!\r\n" );

  // Setup irq
  printf( "[bolthur/kernel -> irq] enable ... " );
  irq_enable();
  printf( "done!\r\n" );

  // Debug breakpoint to kickstart remote gdb
  #if defined( DEBUG )
    debug_breakpoint();
  #endif

  while ( 1 ) {}
}
