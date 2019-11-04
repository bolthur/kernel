
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
#include <arch/arm/v7/cpu.h>

.section .text

// handler
.extern undefined_instruction_handler
.extern software_interrupt_handler
.extern prefetch_abort_handler
.extern data_abort_handler
.extern unused_handler
.extern irq_handler
.extern fast_interrupt_handler

// additional global functions
.extern task_thread_stack

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

  // FIXME: push floating point registers if enabled

  // store sp in r0
  mov r0, sp
.endm

.macro pop_arm_regs mode
  // FIXME: pop floating point registers if enabled

  // switch back to thread that may have changed
  mov r0, sp
  bl task_thread_stack
  mov sp, r0

  // get lr and sp of previous mode
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

_undefined_instruction_handler:
  ex_handler #4, undefined_instruction_handler, CPSR_MODE_SUPERVISOR

_software_interrupt_handler:
  ex_handler #4, software_interrupt_handler, CPSR_MODE_SUPERVISOR

_prefetch_abort_handler:
  ex_handler #4, prefetch_abort_handler, CPSR_MODE_SUPERVISOR

_data_abort_handler:
  ex_handler #8, data_abort_handler, CPSR_MODE_SUPERVISOR

_unused_handler:
  ex_handler #8, unused_handler, CPSR_MODE_SUPERVISOR

_irq_handler:
  ex_handler #4, irq_handler, CPSR_MODE_SUPERVISOR

_fast_interrupt_handler:
  ex_handler #4, fast_interrupt_handler, CPSR_MODE_SUPERVISOR

.global interrupt_vector_table
.balign 32
interrupt_vector_table:
  b start // reset
  b _undefined_instruction_handler // undefined instruction
  b _software_interrupt_handler // software interrupt
  b _prefetch_abort_handler // prefetch abort
  b _data_abort_handler // data abort
  b _unused_handler // unused
  b _irq_handler // irq
  b _fast_interrupt_handler // fiq
