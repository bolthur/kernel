
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

// event handling
IMPORT( event_handle )
// remote debugging
#if defined( REMOTE_DEBUG )
  IMPORT( debug_breakpoint_enable )
  IMPORT( debug_breakpoint_disable )
  IMPORT( debug_watchpoint_enable )
  IMPORT( debug_watchpoint_disable )
#endif
// switch to thread
IMPORT( task_thread_switch_to )
IMPORT( task_thread_current_thread )

// get thread context address
.macro get_thread_context register:req
  // load address of current executing thread
  ldr \register, =task_thread_current_thread
  // get address of pointer
  ldr \register, [ \register ]
  // get context ( first property )
  ldr \register, [ \register ]
.endm

// ensure correct stack alignment before calling any c function
.macro stack_wind register:req, stack:req
  mov \register, \stack
  tst \stack, #4
  subeq \stack, #4
  push { \register }
.endm

// restore stack possibly changed due to stack alignment
.macro stack_unwind register:req, stack:req
  pop { \register }
  mov \stack, \register
.endm

// macro to switch to mode
.macro switch_mode mode:req, fixup:req
  // adjust lr
  sub lr, #\fixup
  // switch mode and push lr and spsr to mode stack
  srsdb sp!, #\mode
  // disable interrupts and switch to mode
  cpsid if, #\mode
.endm

.macro equal_mode_switch offset:req, register:req, mode:req, equal:req, notequal:req
  // save r3 on stack and check for user mode
  push { r3 }
  // load spsr from previous mode
  ldr r3, [ \register, #( \offset + 4 ) ]
  // mask mode only for comparison
  and r3, #CPSR_MODE_MASK
  // compare to passed mode
  cmp r3, #\mode
  // restore r3
  pop { r3 }

  // handle privileged
  bne \notequal
  // handle non privileged
  beq \equal
.endm

.macro privileged_unprivileged_switch offset:req, register:req, privileged:req, unprivileged:req
  // user mode also determined by stack is zero
  cmp \register, #0
  // jump to unprivileged if 0
  beq \unprivileged

  equal_mode_switch \
    \offset, \
    \register, \
    CPSR_MODE_USER, \
    \unprivileged, \
    \privileged
.endm

.macro push_register name:req, mode:req
  // privileged / unprivileged switch
  privileged_unprivileged_switch \
    4, \
    sp, \
    _\name\()_entry_privileged, \
    _\name\()_entry_unprivileged

// non privileged entry
_\name\()_entry_unprivileged:
  // use context from thread structure
  get_thread_context lr

  // save registers
  stmia lr, { r0 - r14 }^

  // get pushed lr and fill it into struct
  ldr r0, [ sp, #0 ]
  str r0, [ lr, #60 ]
  // get pushed spsr and fill it into struct
  ldr r0, [ sp, #4 ]
  str r0, [ lr, #64 ]

  // adjust stack pointer back
  add sp, #8

  // pass 0 as paramter to indicate user entry
  mov r1, #0

  // jump to finished
  b _\name\()_entry_finished

// privileged entry
_\name\()_entry_privileged:
  // allocate space for context structure
  sub sp, #STACK_FRAME_SIZE
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

.macro pop_register name:req, mode:req, stack:req
  // privileged / unprivileged switch
  privileged_unprivileged_switch \
    60, \
    \stack, \
    _\name\()_exit_privileged, \
    _\name\()_exit_unprivileged

// non privileged exit
_\name\()_exit_unprivileged:
  // get context from thread structure
  get_thread_context r0
  // call switch to thread
  b task_thread_switch_to

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

  // load old pc
  ldr r0, [ sp, #60 ]
  // push back onto stack
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

.macro exception_handler fixup:req, handler:req, mode:req, name:req, nested:req
  // switch to svc mode with fixup
  switch_mode \mode, \fixup
  // push register
  push_register \name, \mode

  // remote debugging
  #if defined( REMOTE_DEBUG )
    .ifnc \nested, all
      // ensure stack alignment
      stack_wind r0, sp
      // push r1 and set r0
      push { r1 }
      // disable  breakpoints
      bl debug_breakpoint_disable
      // disable watch points
      bl debug_watchpoint_disable
      // pop r1
      pop { r1 }
      // restore stack adjustments
      stack_unwind r0, sp
    .endif
  #endif

  // ensure stack alignment
  stack_wind r0, sp
  // push r1 and set r0
  push { r1 }
  mov r0, r1
  // call handler routine
  bl \handler
  // pop r1
  pop { r1 }
  // restore stack adjustments
  stack_unwind r0, sp

  // ensure stack alignment
  stack_wind r0, sp
  // push r1 and set r0
  push { r1 }
  mov r0, r1
  // enable interrupts
  .ifc \nested, all
    cpsie if
  .endif
  .ifc \nested, fast
    cpsie f
  .endif
  // handle events
  bl event_handle
  // disable interrupts
  .ifc \nested, all
    cpsid if
  .endif
  .ifc \nested, fast
    cpsid f
  .endif
  // pop r1
  pop { r1 }
  // restore stack adjustments
  stack_unwind r0, sp

  // remote debugging
  #if defined( REMOTE_DEBUG )
    .ifnc \nested, all
      // ensure stack alignment
      stack_wind r0, sp
      // push r1 and set r0
      push { r1 }
      // add breakpoints
      bl debug_breakpoint_enable
      // add watch points
      bl debug_watchpoint_enable
      // pop r1
      pop { r1 }
      // restore stack adjustments
      stack_unwind r0, sp
    .endif
  #endif

  // restore registers for return
  pop_register \name, \mode, r1
  // return from exception
  rfeia sp!
.endm
