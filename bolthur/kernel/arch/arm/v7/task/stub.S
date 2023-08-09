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

#define ASSEMBLER_FILE 1
#include "../../../../assembly.h"
#include "../cpu.h"
#include "../interrupt/handler.S"

EXPORT( task_thread_switch_to )
task_thread_switch_to:
  // switch to svc mode
  cpsid if, #CPSR_MODE_SUPERVISOR
  // push parameter into fp
  mov fp, r0
  // load return from passed context
  ldr lr, [ fp, #PC_OFFSET ]
  // load spsr from passed context
  ldr r0, [ fp, #SPSR_OFFSET ]
  // set spsr for returning
  msr SPSR_cxsf, r0
  // restore floating point registers
  pop_fpu_register fp
  // restore registers and switch to process
  ldm fp, { r0 - r14 }^
  nop
  // return to user mode
  movs pc, lr
