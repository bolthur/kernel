
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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
#include <irq.h>
#include <isrs.h>
#include <timer.h>
#include <platform.h>
#include <debug.h>
#include <serial.h>

#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif
void kernel_main() {
  // enable tty for output
  tty_init();

  printf(
    "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
    " ______     __         __         ______     ______        __    __     __     ______     ______  ",
    "/\\  __ \\   /\\ \\       /\\ \\       /\\  ___\\   /\\  ___\\      /\\ \"-./  \\   /\\ \\   /\\  ___\\   /\\__  _\\ ",
    "\\ \\  __ \\  \\ \\ \\____  \\ \\ \\____  \\ \\  __\\   \\ \\___  \\     \\ \\ \\-./\\ \\  \\ \\ \\  \\ \\___  \\  \\/_/\\ \\/ ",
    " \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_____\\  \\ \\_____\\  \\/\\_____\\     \\ \\_\\ \\ \\_\\  \\ \\_\\  \\/\\_____\\    \\ \\_\\ ",
    "  \\/_/\\/_/   \\/_____/   \\/_____/   \\/_____/   \\/_____/      \\/_/  \\/_/   \\/_/   \\/_____/     \\/_/ "
  );

  // FIXME: Find correct place if necessary
  /*printf( "[mist-system/kernel -> platform] initialize ... " );
  platform_init();
  printf( "done!\r\n" );*/

  // Setup isrs
  printf( "[mist-system/kernel -> isrs] initialize ... " );
  isrs_init();
  printf( "done!\r\n" );

  // Setup memory management
  printf( "[mist-system/kernel -> memory] initialize ... [" );
  // FIXME: Setup physical memory management
  printf( " physical, " );
  // FIXME: Setup virtual memory management
  printf( " virtual, " );
  // FIXME: Setup heap
  printf( " heap ] " );
  printf( "done!\r\n" );

  // FIXME: Setup event system
  printf( "[mist-system/kernel -> event] initialize ... " );
  printf( "done!\r\n" );

  // Setup debug if enabled
  #if defined( DEBUG )
    printf( "[mist-system/kernel -> debug] initialize ... " );
    debug_init();
    printf( "done!\r\n" );
  #endif

  // Setup timer
  printf( "[mist-system/kernel -> timer] initialize ... " );
  timer_init();
  printf( "done!\r\n" );

  // Setup irq
  printf( "[mist-system/kernel -> irq] enable ... " );
  irq_enable();
  printf( "done!\r\n" );

  // Debug breakpoint to kickstart remote gdb
  #if defined( DEBUG )
    debug_breakpoint();
  #endif

  while ( 1 ) {}
}
