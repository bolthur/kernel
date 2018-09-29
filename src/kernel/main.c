
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

  // Setup isrs
  printf( "[mist-system/kernel -> isrs] initialize ...\r\n" );
  isrs_init();

  // Setup timer
  printf( "[mist-system/kernel -> timer] initialize ...\r\n" );
  timer_init();

  // Setup debug if enables
  #ifdef DEBUG
    printf( "[mist-system/kernel -> debug] initialize ...\r\n" );
    debug_init();
  #endif

  // Setup irq
  printf( "[mist-system/kernel -> irq] initialize ...\r\n" );
  irq_init();

  asm("swi 3");

  while ( 1 ) {}
}
