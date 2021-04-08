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

#include <sys/types.h>
#include <sys/mman.h>
#include <stddef.h>

__attribute__((weak))
void _start( void ) {
  void *addr = NULL;
  register unsigned r0 __asm__( "r0" ) = ( unsigned )NULL;
  register unsigned r1 __asm__( "r1" ) = sizeof( uint32_t ) * 4;
  register unsigned r2 __asm__( "r2" ) = PROT_READ | PROT_WRITE;
  register unsigned r3 __asm__( "r3" ) = MAP_PRIVATE;
  register signed r4 __asm__( "r4" ) = -1;
  register unsigned r5 __asm__( "r5" ) = 0;
  __asm__ __volatile__( "\n\
      svc #21"
        : "+r"( r0 ) // addr
        : "r"( r1 ), // len
          "r"( r2 ), // prot
          "r"( r3 ), // flags
          "r"( r4 ), // file descriptor
          "r"( r5 ) // offset
        : "cc"
    );
  /*
   *
  int result;
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    mov r1, %[r1] \n\
    svc #22 \n\
    mov %[r2], r0"
      : [ r2 ]"=r"( result )
      : [ r0 ]"r"( addr ),
        [ r1 ]"r"( len )
      : "cc"
  );
  return result;
   */
  for( ;; ) {}
}
