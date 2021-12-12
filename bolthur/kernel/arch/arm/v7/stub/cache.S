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

#define ASSEMBLER_FILE 1
#include <assembly.h>

.section .text.boot

EXPORT( cache_startup_setup )
cache_startup_setup:
  // preserve r0 - r6
  push {r0 - r6}
  // set ccselr to 0
  mov r0, #0
  mcr p15, 2, r0, c0, c0, 0
  isb
  // fetch ccselr again
  mrc p15, 1, r0, c0, c0, 0
  // extract max number of index size
  movw r1, #0x7FFF
  and r2, r1, r0, lsr #13
  // num ways - 1
  mov r1, #0x3FF
  and r3, r1, r0, lsr #3
  // num sets
  add r2, r2, #1
  // set shift
  and r0, r0, #0x7
  add r0, r0, #4
  // way shift
  clz r1, r3
  // num ways
  add r4, r3, #1
1:
  // num sets - 1
  sub r2, r2, #1
  // tmp = num ways
  mov r3, r4
2:
  // tmp - 1
  subs r3, r3, #1
  mov r5, r3, lsr r1
  mov r6, r2, lsr r0
  // reg = ( tmp << way shift ) | ( num sets << set shift )
  orr r5, r5, r6
  mcr p15, 0, r5, c7, c6, 2
  bgt 2b
  cmp r2, #0
  bgt 1b
  // invalidate instruction cache
  mov r0, #0
  mcr p15, 0, r0, c7, c5, 0
  // flush branch target cache
  mcr p15, 0, r0, c7, c5, 6
  // invalidate mmu
  mcr p15, 0, r0, c8, c7, 0
  // wait until it's done
  dsb
  isb
  // restore and return
  pop {r0-r6}
  bx lr

.section .text

EXPORT( cache_invalidate_data )
cache_invalidate_data:
  // preserve r0 - r6
  push {r0 - r6}
  // set ccselr to 0
  mov r0, #0
  mcr p15, 2, r0, c0, c0, 0
  isb
  // fetch ccselr again
  mrc p15, 1, r0, c0, c0, 0
  // extract max number of index size
  movw r1, #0x7FFF
  and r2, r1, r0, lsr #13
  // num ways - 1
  mov r1, #0x3FF
  and r3, r1, r0, lsr #3
  // num sets
  add r2, r2, #1
  // set shift
  and r0, r0, #0x7
  add r0, r0, #4
  // way shift
  clz r1, r3
  // num ways
  add r4, r3, #1
1:
  // num sets - 1
  sub r2, r2, #1
  // tmp = num ways
  mov r3, r4
2:
  // tmp - 1
  subs r3, r3, #1
  mov r5, r3, lsr r1
  mov r6, r2, lsr r0
  // reg = ( tmp << way shift ) | ( num sets << set shift )
  orr r5, r5, r6
  mcr p15, 0, r5, c7, c6, 2
  bgt 2b
  cmp r2, #0
  bgt 1b
  // invalidate instruction cache
  mov r0, #0
  mcr p15, 0, r0, c7, c5, 0
  // flush branch target cache
  mcr p15, 0, r0, c7, c5, 6
  // invalidate mmu
  mcr p15, 0, r0, c8, c7, 0
  // wait until it's done
  dsb
  isb
  // restore and return
  pop {r0-r6}
  bx lr
