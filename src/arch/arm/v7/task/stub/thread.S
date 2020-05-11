
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <core/assembly.h>
#include <arch/arm/v7/cpu.h>

IMPORT( task_thread_current_thread )

EXPORT( task_thread_switch_to )
task_thread_switch_to:
  // set fp
  mov fp, r0
  // load pc/lr
  ldr lr, [ fp, #60 ]
  // load initial context
  ldr r3, =task_thread_current_thread
  ldr r3, [ r3 ]
  ldr r3, [ r3, #4 ]
  // load pc/lr from initial context
  ldr r3, [ r3, #60 ]
  // load spsr
  ldr r0, [ fp, #64 ]
  // set spsr
  msr spsr_cxsf, r0
  // restore registers
  ldmia fp, { r0 - r14 }^
  nop
  // return to process
  movs pc, lr
