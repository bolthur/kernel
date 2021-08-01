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
#include <arch/arm/v7/cpu.h>

EXPORT( task_thread_switch_to )
task_thread_switch_to:
  // cache parameter at fp register
  mov fp, r0
  // switch to system mode first
  cpsid if, #CPSR_MODE_SYSTEM
  // load spsr from passed context
  ldr r0, [ fp, #64 ]
  // set spsr used when returning
  msr SPSR_cxsf, r0
  // restore registers and switch to process
  ldm fp, { r0 - r15 }^
  nop
