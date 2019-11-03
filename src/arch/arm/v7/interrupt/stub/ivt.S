
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

.macro push_arm_regs mode, tcb
  // push main registers

  // save r0
  push { r0 }
  // load tcb into r0
  ldr r0, =\tcb
  // push main registers
  stmia r0, { r0 - r12 }
  // restore r0
  pop { r0 }

  // save r1
  push { r1 }
  // load tcb into r1
  ldr r1, =\tcb
  // push r0 to tcb
  str r0, [r1, #0]
  // restore r1
  pop { r1 }

  // load tcb into r4
  ldr r4, =\tcb

  // get pushed lr and fill it into struct
  ldr r0, [sp, #4]
  str r0, [r4, #60]

  // load r0 with sp beyond pushed lr
  ldr r0, [sp, #8]
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
  str r1, [r4, #52]
  str r2, [r4, #56]
  str r3, [r4, #64]

  // FIXME: push floating point registers if enabled
.endm

.macro pop_arm_regs mode, tcb
  // FIXME: pop floating point registers if enabled

  // load tcb into r3
  ldr r3, =\tcb

  // get lr and sp of previous mode
  ldr r1, [r3, #52]
  ldr r2, [r3, #56]
  // switch to previous mode
  ldr r0, [r3, #64]
  msr cpsr_c, r0
  // restore sp and lr
  mov sp, r1
  mov lr, r2

  // switch again to passed mode
  cps \mode

  // load old pc and fixup
  ldr r0, [r3, #60]
  str r0, [r3, #72]

  // restore previous state
  ldmia r3, {r0-r12}
.endm

.macro ex_handler lr_fixup, handler, mode, tcb
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
  push_arm_regs \mode, \tcb
  bl \handler
  pop_arm_regs \mode, \tcb

  // restore lr
  pop {lr}
  // enable interrupts again
  cpsie if
  // return from exception
  rfeia sp!
.endm

// thread control blocks
.extern tcb_undefined
.extern tcb_software
.extern tcb_prefetch
.extern tcb_data
.extern tcb_unused
.extern tcb_irq
.extern tcb_fiq

// handler
.extern undefined_instruction_handler
.extern software_interrupt_handler
.extern prefetch_abort_handler
.extern data_abort_handler
.extern unused_handler
.extern irq_handler
.extern fast_interrupt_handler

_undefined_instruction_handler:
  ex_handler #4, undefined_instruction_handler, CPSR_MODE_SUPERVISOR, tcb_undefined

_software_interrupt_handler:
  ex_handler #4, software_interrupt_handler, CPSR_MODE_SUPERVISOR, tcb_software

_prefetch_abort_handler:
  ex_handler #4, prefetch_abort_handler, CPSR_MODE_SUPERVISOR, tcb_prefetch

_data_abort_handler:
  ex_handler #8, data_abort_handler, CPSR_MODE_SUPERVISOR, tcb_data

_unused_handler:
  ex_handler #8, unused_handler, CPSR_MODE_SUPERVISOR, tcb_unused

_irq_handler:
  ex_handler #4, irq_handler, CPSR_MODE_SUPERVISOR, tcb_irq

_fast_interrupt_handler:
  ex_handler #4, fast_interrupt_handler, CPSR_MODE_SUPERVISOR, tcb_fiq

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
