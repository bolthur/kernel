
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

.section .text

// handler
IMPORT( undefined_instruction_handler )
IMPORT( software_interrupt_handler )
IMPORT( prefetch_abort_handler )
IMPORT( data_abort_handler )
IMPORT( unused_handler )
IMPORT( interrupt_handler )
IMPORT( fast_interrupt_handler )
// event handling
IMPORT( event_handle )
// switch to thread
IMPORT( switch_to_thread )


// ensure correct stack alignment before calling any c function
.macro stack_wind register, stack
  mov \register, \stack
  tst \stack, #4
  subeq \stack, #4
  push { \register }
.endm

// restore stack possibly changed due to stack alignment
.macro stack_unwind register, stack
  pop { \register }
  mov \stack, \register
.endm

// macro to switch to mode
.macro switch_mode mode, fixup
  // adjust lr
  sub lr, #\fixup
  // switch mode and push lr and spsr to mode stack
  srsdb sp!, #\mode
  // disable interrupts and switch to mode
  cpsid if, #\mode
.endm

.macro privileged_unprivileged_switch offset, register, privileged, unprivileged
  // save r3 on stack and check for user mode
  push { r3 }
  // load spsr from previous mode
  ldr r3, [ \register, #( \offset + 4 ) ]
  // mask mode only for comparison
  and r3, #CPSR_MODE_MASK
  // compare to user mode
  cmp r3, #CPSR_MODE_USER
  // restore r3
  pop { r3 }

  // handle privileged
  bne \privileged

  // handle non privileged
  beq \unprivileged
.endm

.macro push_register name, mode
  // privileged / unprivileged switch
  privileged_unprivileged_switch \
    4, \
    sp, \
    _\name\()_entry_privileged, \
    _\name\()_entry_unprivileged

// non privileged entry
_\name\()_entry_unprivileged:
  // store r13 and r14 usermode
  stmfd sp, { r13 }^
  nop
  // update stack pointer
  sub sp, #4

  // load lr from stack
  ldr lr, [ sp ]
  // allocate space for context structure
  sub lr, lr, #STACK_FRAME_SIZE
  // save registers
  stmia lr, { r0 - r14 }^

  // get pushed lr and fill it into struct
  ldr r0, [ sp, #4 ]
  str r0, [ lr, #60 ]
  // get pushed spsr and fill it into struct
  ldr r0, [ sp, #8 ]
  str r0, [ lr, #64 ]

  // adjust stack pointer back
  add sp, #12

  // save fp in first register
  mov r1, lr

  // jump to finished
  b _\name\()_entry_finished

// privileged entry
_\name\()_entry_privileged:
  // allocate space for context structure
  sub sp, sp, #STACK_FRAME_SIZE
  // save registers
  stmia sp, { r0 - r12 }
  // get pushed lr and fill it into struct
  ldr r0, [ sp, #68 ]
  str r0, [ sp, #60 ]
  // get pushed spsr and fill it into struct
  ldr r0, [ sp, #72 ]
  str r0, [ sp, #64 ]

  // get pushed spsr for mode switch
  ldr r0, [ sp, #72 ]
  // switch to previous mode
  orr r0, r0, #( CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
  msr cpsr_c, r0
  // get sp and lr
  mov r1, sp
  mov r2, lr
  // switch mode again
  cps \mode
  // push sp, lr
  str r1, [ sp, #52 ]
  str r2, [ sp, #56 ]

  // save sp in first register
  mov r1, sp

  // jump to finished
  b _\name\()_entry_finished

// finished entry
_\name\()_entry_finished:
.endm

.macro pop_register name, mode, stack
  // privileged / unprivileged switch
  privileged_unprivileged_switch \
    64, \
    \stack, \
    _\name\()_exit_privileged, \
    _\name\()_exit_unprivileged

// non privileged exit
_\name\()_exit_unprivileged:
  // set r0
  mov r0, \stack
  // call switch to thread
  bl switch_to_thread
  // jump to finished
  b _\name\()_exit_finished

// privileged exit
_\name\()_exit_privileged:
  // get lr and sp of previous mode
  ldr r1, [ sp, #52 ]
  ldr r2, [ sp, #56 ]
  // switch to previous mode
  ldr r0, [ sp, #64 ]
  orr r0, r0, #( CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
  msr cpsr_c, r0
  // restore sp and lr
  mov sp, r1
  mov lr, r2

  // switch again to passed mode
  cps \mode

  // load old pc and fixup
  ldr r0, [ sp, #60 ]
  str r0, [ sp, #68 ]
  // restore previous state
  ldmia sp, { r0 - r12 }
  // correct stack pointer again
  add sp, #STACK_FRAME_SIZE

  // jump to finished
  b _\name\()_exit_finished

// finished exit
_\name\()_exit_finished:
.endm

.macro ex_handler fixup, handler, mode, name
  // switch to svc mode with fixup
  switch_mode \mode, \fixup
  // push register
  push_register \name, \mode

  // ensure stack alignment
  stack_wind r0, r1
  // call handler routine
  bl \handler
  // restore stack adjustments
  stack_unwind r0, r1

  // ensure stack alignment
  stack_wind r0, r1
  // enable interrupts
  cpsie if
  // handle events
  bl event_handle
  // disable interrupts
  cpsid if
  // restore stack adjustments
  stack_unwind r0, r1

  // restore registers
  pop_register \name, \mode, r1
  // enable interrupts again
  cpsie if
  // return from exception
  rfeia sp!
.endm

_undefined_instruction_handler:
  ex_handler 4, undefined_instruction_handler, CPSR_MODE_SUPERVISOR, undefined

_software_interrupt_handler:
  ex_handler 4, software_interrupt_handler, CPSR_MODE_SUPERVISOR, software

_prefetch_abort_handler:
  ex_handler 4, prefetch_abort_handler, CPSR_MODE_SUPERVISOR, prefetch_abort

_data_abort_handler:
  ex_handler 8, data_abort_handler, CPSR_MODE_SUPERVISOR, data_abort

_interrupt_handler:
  ex_handler 4, interrupt_handler, CPSR_MODE_SUPERVISOR, interrupt

_fast_interrupt_handler:
  ex_handler 4, fast_interrupt_handler, CPSR_MODE_SUPERVISOR, fast_interrupt

EXPORT( interrupt_vector_table )
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
