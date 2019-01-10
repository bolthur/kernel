
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
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

#define ASSEMBLER_FILE 1
#include "arch/arm/v7/cpu.h"

.section .text

.macro push_arm_regs mode
  // allocate space for context structure
  sub sp, sp, #STACK_FRAME_SIZE
  // push main registers
  stmia sp, {r0-r12}

  // get pushed lr and fill it into struct
  ldr r0, [sp, #72]
  str r0, [sp, #60]

  // load r0 with sp beyond pushed lr
  ldr r0, [sp, #76]
  mov r3, r0

  // switch to previous mode
  orr r0, r0, #( CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
  msr cpsr_c, r0

  // get sp and lr
  mov r1, sp
  mov r2, lr

  // switch mode again
  cps \mode

  // push lr, sp, spsr
  str r1, [sp, #52]
  str r2, [sp, #56]
  str r3, [sp, #64]

  // store sp in r0
  mov r0, sp
.endm

.macro pop_arm_regs mode
  // restore lr and sp
  ldr r1, [sp, #52]
  ldr r2, [sp, #56]

  // switch to previous mode
  ldr r0, [sp, #64]
  orr r0, r0, #( CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
  msr cpsr_c, r0

  // restore sp and lr
  mov sp, r1
  mov lr, r2

  // switch again to passed mode
  cps \mode

  // load old pc and fixup
  ldr r0, [sp, #60]
  str r0, [sp, #72]

  // restore previous state
  ldmia sp, {r0-r12}

  // correct stack pointer again
  add sp, #STACK_FRAME_SIZE
.endm

.macro ex_handler lr_fixup, handler, mode
  // construct return address
  sub lr, lr, \lr_fixup

  // store return state on different stack
  srsdb sp!, \mode
  // switch to passed mode
  cps \mode
  // disable interrupts
  cpsid if
  // save lr
  push {lr}

  // push registers, execute handler and restore again
  push_arm_regs \mode
  bl \handler
  pop_arm_regs \mode

  // restore lr
  pop {lr}
  // enable interrupts again
  cpsie if
  // return from exception
  rfeia sp!
.endm

.extern undefined_instruction_handler
.extern software_interrupt_handler
.extern prefetch_abort_handler
.extern data_abort_handler
.extern unused_handler
.extern irq_handler
.extern fast_interrupt_handler

.global _undefined_instruction_handler
_undefined_instruction_handler:
  ex_handler #4, undefined_instruction_handler, CPSR_MODE_SUPERVISOR

.global _software_interrupt_handler
_software_interrupt_handler:
  ex_handler #4, software_interrupt_handler, CPSR_MODE_SUPERVISOR

.global _prefetch_abort_handler
_prefetch_abort_handler:
  ex_handler #4, prefetch_abort_handler, CPSR_MODE_SUPERVISOR

.global _data_abort_handler
_data_abort_handler:
  ex_handler #8, data_abort_handler, CPSR_MODE_SUPERVISOR

.global _unused_handler
_unused_handler:
  ex_handler #8, unused_handler, CPSR_MODE_SUPERVISOR

.global _irq_handler
_irq_handler:
  ex_handler #4, irq_handler, CPSR_MODE_SUPERVISOR

.global _fast_interrupt_handler
_fast_interrupt_handler:
  ex_handler #4, fast_interrupt_handler, CPSR_MODE_SUPERVISOR
