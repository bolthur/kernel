/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

// FIXME: TEST WITH MULTIPLE THREADS!

char str[] = "\r\nexit process\r\n";

void _start( void ) {
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str )
  );
  // just exit :)
  __asm__ __volatile__( "svc #2" ::: "cc" );
}
