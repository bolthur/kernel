
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <kernel/assembly.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/stack.h>

.section .text

EXPORT( arch_start )
arch_start:
  // setup of other modes stacks not necessary as all exceptions use supervisor mode

  // switch to supervisor mode and setup stack
  msr cpsr_c, #( CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_supervisor_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // branch and link to kernel main entry
  ldr r3, =kernel_main
  blx r3  // blx may switch to Thumb mode, depending on the target address

halt:
  wfe // equivalent of x86 HLT instruction
  b halt
