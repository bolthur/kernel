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

.section .text

EXPORT( cache_enable_stub )
cache_enable_stub:
  push { r0 - r2, lr }
  ldr r1, [ r0 ]
  cmp r1, #0
  bne _cache_enable_finish
  // read status control register
  mrc p15, 0, r1, c1, c0, 0
  // disable instruction cache
  bic r1, r1, #( 1 << 12 )
  // disable data cache
  bic r1, r1, #( 1 << 2 )
  // disable branch prediction
  bic r1, r1, #( 1 << 11 )
  // push back
  mcr p15, 0, r1, c1, c0, 0
  dsb
  isb

  push { r0 }
  // clean cache
  bl cache_clean_data
  // invalidate cache ( handles instruction cache flush, branch target flush and mmu invalidation )
  bl cache_invalidate_data
  pop { r0 }

  // read status control register
  mrc p15, 0, r1, c1, c0, 0
  // enable instruction cache
  orr r1, r1, #( 1 << 12 )
  // enable data cache
  orr r1, r1, #( 1 << 2 )
  // enable branch prediction
  orr r1, r1, #( 1 << 11 )
  // push back
  mcr p15, 0, r1, c1, c0, 0
  dsb
  isb
  // write boolean to parameter
  mov r1, #1
  str r1, [ r0 ]
_cache_enable_finish:
  pop { r0 - r2, lr }
  bx lr

EXPORT( cache_invalidate_data )
cache_invalidate_data:
  stmfd sp!, { r4-r5, r9-r11, lr }
  // ensure ordering with previous memory accesses
  dmb
  // read clidr
  mrc p15, 1, r0, c0, c0, 1
  // move LoC into position
  mov r3, r0, lsr #23
  // extract LoC*2 from clidr
  ands  r3, r3, #7 << 1
  // if loc is 0, then no need to clean
  beq inval_finished
  // start clean at cache level 0
  mov r10, #0
inval_levels:
  // work out 3x current cache level
  add r2, r10, r10, lsr #1
  // extract cache type bits from clidr
  mov r1, r0, lsr r2
  // mask of the bits for current cache only
  and r1, r1, #7
  // see what cache we have at this level
  cmp r1, #2
  // skip if no cache, or just i-cache
  blt inval_skip
  // select current cache level in cssr
  mcr p15, 2, r10, c0, c0, 0
  // isb to synchronize the new cssr&csidr
  isb
  // read the new csidr
  mrc p15, 1, r1, c0, c0, 0
  // extract the length of the cache lines
  and r2, r1, #7
  // add 4 (line length offset)
  add r2, r2, #4
  movw  r4, #0x3ff
  // find maximum number on the way size
  ands  r4, r4, r1, lsr #3
  // find bit position of way size increment
  clz r5, r4
  movw  r7, #0x7fff
  // extract max number of the index size
  ands  r7, r7, r1, lsr #13
inval_loop1:
  // create working copy of max index
  mov r9, r7
inval_loop2:
  // factor way and cache number into r11
  orr r11, r10, r4, lsl r5
  // factor index number into r11
  orr r11, r11, r9, lsl r2
  // invalidate by set/way
  mcr p15, 0, r11, c7, c6, 2
  // decrement the index
  subs  r9, r9, #1
  bge inval_loop2
  // decrement the way
  subs  r4, r4, #1
  bge inval_loop1
inval_skip:
  // increment cache number
  add r10, r10, #2
  cmp r3, r10
  bgt inval_levels
inval_finished:
  // swith back to cache level 0
  mov r10, #0
  // select current cache level in cssr
  mcr p15, 2, r10, c0, c0, 0
  dsb st
  isb
  ldmfd sp!, { r4-r5, r9-r11, lr }
  bx  lr

EXPORT( cache_clean_data )
cache_clean_data:
  stmfd sp!, {r4-r5, r7, r9-r11, lr}
  // ensure ordering with previous memory accesses
  dmb
  // read clidr
  mrc p15, 1, r0, c0, c0, 1
  // move LoC into position
  mov r3, r0, lsr #23
  // extract LoC*2 from clidr
  ands  r3, r3, #7 << 1
  // if loc is 0, then no need to clean
  beq finished
start_flush_levels:
  // start clean at cache level 0
  mov r10, #0
flush_levels:
  // work out 3x current cache level
  add r2, r10, r10, lsr #1
  // extract cache type bits from clidr
  mov r1, r0, lsr r2
  // mask of the bits for current cache only
  and r1, r1, #7
  // see what cache we have at this level
  cmp r1, #2
  // skip if no cache, or just i-cache
  blt skip
  // select current cache level in cssr
  mcr p15, 2, r10, c0, c0, 0
  // isb to synchronize the new cssr&csidr
  isb
  // read the new csidr
  mrc p15, 1, r1, c0, c0, 0
  // extract the length of the cache lines
  and r2, r1, #7
  // add 4 (line length offset)
  add r2, r2, #4
  movw  r4, #0x3ff
  // find maximum number on the way size
  ands  r4, r4, r1, lsr #3
  // find bit position of way size increment
  clz r5, r4
  movw  r7, #0x7fff
  // extract max number of the index size
  ands  r7, r7, r1, lsr #13
loop1:
  // create working copy of max index
  mov r9, r7
loop2:
  // factor way and cache number into r11
  orr r11, r10, r4, lsl r5
  // factor index number into r11
  orr r11, r11, r9, lsl r2
  // clean & invalidate by set/way
  mcr p15, 0, r11, c7, c14, 2
  // decrement the index
  subs  r9, r9, #1
  bge loop2
  // decrement the way
  subs  r4, r4, #1
  bge loop1
skip:
  // increment cache number
  add r10, r10, #2
  cmp r3, r10
  bgt flush_levels
finished:
  // swith back to cache level 0
  mov r10, #0
  // select current cache level in cssr
  mcr p15, 2, r10, c0, c0, 0
  dsb st
  isb
  ldmfd sp!, { r4-r5, r7, r9-r11, lr }
  bx  lr
