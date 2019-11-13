
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

// macro to switch to mode
.macro switch_mode mode, fixup
  // adjust lr
  sub lr, #\fixup
  // switch mode and push lr and spsr to mode stack
  srsdb sp!, #\mode
  // disable interrupts and switch to mode
  cpsid if, #\mode
.endm

.macro privileged_unprivileged_switch offset, privileged, unprivileged, pre
  // save r3 on stack and check for user mode
  push { r3 }
  // load spsr from previous mode
  ldr r3, [ sp, #( \offset + 4 ) ]
  // mask interrupts on return
  orr r3, #( CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT )
  // push back value
  str r3, [ sp, #( \offset + 4 ) ]
  // mask mode only for comparison
  and r3, #CPSR_MODE_MASK
  // compare to user mode
  cmp r3, #CPSR_MODE_USER
  // restore r3
  pop { r3 }

  // allocate space for context structure
  .ifnb \pre
    sub sp, sp, #STACK_FRAME_SIZE
  .endif

  // handle privileged
  bne \privileged

  // handle non privileged
  beq \unprivileged
.endm

.macro push_register name
  // privileged / unprivileged switch
  privileged_unprivileged_switch \
    4, \
    _\name\()_entry_privileged, \
    _\name\()_entry_unprivileged, \
    1

// non privileged entry
_\name\()_entry_unprivileged:
  stmia sp, {r0-r14}^
  b _\name\()_entry_finished

// privileged entry
_\name\()_entry_privileged:
  stmia sp, {r0-r14}
  b _\name\()_entry_finished

// finished entry
_\name\()_entry_finished:
  // get pushed lr and fill it into struct
  ldr r0, [ sp, #68 ]
  str r0, [ sp, #60 ]
  // get pushed spsr and fill it into struct
  ldr r0, [ sp, #72 ]
  str r0, [ sp, #64 ]
.endm

.macro pop_register name
  // restore lr from stack
  ldr r0, [ sp, #60 ]
  str r0, [ sp, #68 ]
  // restore spsr from stack
  ldr r0, [ sp, #64 ]
  str r0, [ sp, #72 ]

  // privileged / unprivileged switch
  privileged_unprivileged_switch \
    64, \
    _\name\()_exit_privileged, \
    _\name\()_exit_unprivileged

// non privileged exit
_\name\()_exit_unprivileged:
  /// FIXME: RESTORE REGISTERS
  b _\name\()_exit_finished

// privileged exit
_\name\()_exit_privileged:
  // restore previous state
  ldmia sp, { r0 - r14 }
  b _\name\()_exit_finished

// finished exit
_\name\()_exit_finished:
.endm

// see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka13552.html
.macro ex_handler fixup, handler, mode, name
  // switch to svc mode with fixup
  switch_mode \mode, \fixup
  // push register
  push_register \name
  // ensure stack alignment
  stack_wind r0
  // call handler routine
  bl \handler
  // restore stack adjustments
  stack_unwind r0
  // enable interrupts
  cpsie if
  // ensure stack alignment
  stack_wind r0
  // handle events
  bl event_handle
  // disable interrupts
  cpsid if
  // restore stack adjustments
  stack_unwind r0
  // restore registers
  pop_register \name
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
