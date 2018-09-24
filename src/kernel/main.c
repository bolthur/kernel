
/**
 * mist-system/kernel
 * Copyright (C) 2017 mist-system project.
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
#include <interrupt.h>
#include <timer.h>
#include <platform.h>

void kernel_main() {
  tty_init();

  printf(
    "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
    " ______     __         __         ______     ______        __    __     __     ______     ______  ",
    "/\\  __ \\   /\\ \\       /\\ \\       /\\  ___\\   /\\  ___\\      /\\ \"-./  \\   /\\ \\   /\\  ___\\   /\\__  _\\ ",
    "\\ \\  __ \\  \\ \\ \\____  \\ \\ \\____  \\ \\  __\\   \\ \\___  \\     \\ \\ \\-./\\ \\  \\ \\ \\  \\ \\___  \\  \\/_/\\ \\/ ",
    " \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_____\\  \\ \\_____\\  \\/\\_____\\     \\ \\_\\ \\ \\_\\  \\ \\_\\  \\/\\_____\\    \\ \\_\\ ",
    "  \\/_/\\/_/   \\/_____/   \\/_____/   \\/_____/   \\/_____/      \\/_/  \\/_/   \\/_/   \\/_____/     \\/_/ ",
    "mist-system/kernel booting!"
  );

  printf(
    "Hex test: 0x%08x\r\nHex test: 0x%8x\r\nNumber test: %d - %i = %d\r\nSecond number test: %u\r\n",
    0xBAD, 0xBAD, 10, 20, -10, -10
  );

  printf( "Initializing interrupts ... " );
  irq_init();
  printf( "done!\r\n" );

  printf( "Initializing timer ... " );
  timer_init();
  printf( "done!\r\n" );

  printf( "Enabling interrupts ... " );
  irq_enable();
  printf( "done!\r\n" );

  while ( 1 ) {}
}
