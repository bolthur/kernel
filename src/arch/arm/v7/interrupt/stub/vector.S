
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
.extern interrupt_handler
.extern fast_interrupt_handler


// ensure correct stack alignment before calling any c function
.macro stack_wind register
  mov \register, sp
  tst sp, #4
  subeq sp, #4
  push {\register}
.endm

// restore stack possibly changed due to stack alignment
.macro stack_unwind register
  pop {\register}
  mov sp, \register
.endm

.macro push_arm_regs mode
  // allocate space for context structure
  sub sp, sp, #STACK_FRAME_SIZE

  // push main registers of previous mode without switching
  stmia sp, {r0-r12}
  // FIXME: USE INSTRUCTION BELOW ONLY WHEN INTERRUPT HAS BEEN TRIGGERED
  // FROM NON SVC MODE. TEST AGAINST MODE AND HANDLE DIFFERENTLY IF INTERRUPT
  // OCCURRED ON PRIVILEGED LEVEL
  // stmia sp, {r0-r15}^

  // get pushed pc/lr and fill it into struct
  ldr r0, [sp, #72]
  str r0, [sp, #60]
  // get pushed spsr and fill it into struct
  ldr r0, [sp, #76]
  str r0, [sp, #64]

  // FIXME: CHECK MODE AGAINST USER MODE. IF PREVIOUS MODE IS USER MODE, SWITCH
  // TO SYSTEM MODE, SET LR AND SP AND SWITCH BACK INSTEAD OF JUST SWITCH TO
  // PREVIOUS MODE. WHEN PREVIOUS MODE IS SVC MODE ( ONLY SIMPLE KERNEL IDLE
  // THREADS ) SWITCH TO SVC mode
  // load r0 with spsr beyond pushed lr
  // get pushed spsr and fill it into struct
  ldr r0, [sp, #76]
  // switch to previous mode
  orr r0, r0, #( CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
  msr cpsr_c, r0
  // get sp and lr
  mov r1, sp
  mov r2, lr
  // switch mode again
  cps \mode
  // push lr, sp
  str r1, [sp, #52]
  str r2, [sp, #56]



  // FIXME: push floating point registers if enabled

  // store sp in r0
  mov r0, sp
.endm

.macro pop_arm_regs mode
  // FIXME: pop floating point registers if enabled

  // FIXME: CHECK WHETHER STACK OVERWRITE IS REALLY NECESSARY
  // switch back to thread that may have changed
  // mov r0, sp
  // bl task_thread_stack
  // mov sp, r0

  // FIXME: CHECK MODE AGAINST USER MODE. IF PREVIOUS MODE IS USER MODE, SWITCH
  // TO SYSTEM MODE, SET LR AND SP AND SWITCH BACK INSTEAD OF JUST SWITCH TO
  // PREVIOUS MODE. WHEN PREVIOUS MODE IS SVC MODE ( ONLY SIMPLE KERNEL IDLE
  // THREADS ) SWITCH TO SVC mode
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

// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka13552.html
.macro ex_handler lr_fixup, handler, mode
  // construct return address
  sub lr, lr, \lr_fixup
  // store lr and spsr on stack of mode to switch to
  srsdb sp!, \mode
  // switch to passed mode with interrupts disabled
  cpsid if, \mode
  // save lr
  push {lr}

  // push registers
  push_arm_regs \mode
  // ensure stack alignment
  stack_wind r0
  // call c handler
  bl \handler
  // restore stack adjustments
  stack_unwind r0
  // restore registers
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

_interrupt_handler:
  ex_handler #4, interrupt_handler, CPSR_MODE_SUPERVISOR

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
  nop // unused
  b _interrupt_handler // irq
  b _fast_interrupt_handler // fiq
