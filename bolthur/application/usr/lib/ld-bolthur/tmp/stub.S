/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

.global dl_resolve_stub
.extern dl_resolve_lazy

dl_resolve_stub:
  // ip contains &GOT[n+3] (pointer to function)
  // lr points to &GOT[2]
  // push argument registers to stack
  stmdb sp!, {r0-r3}
  // extract handle pointer from stack
  ldr r0, [ lr, #-4 ]
  // extract symbol
  sub r1, ip, lr
  sub r1, #4
  add r1, r1
  // call to resolve lazy
  bl dl_resolve_lazy
  // save return
  mov r12, r0
  // restore arguments for original function to call
  ldmia sp!, {r0, r1, r2, r3, lr}
  // branch to looked up function
  mov pc, r12
