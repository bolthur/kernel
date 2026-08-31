/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
.macro stack_wind register:req, additional:req, stack:req
  // test alignment
  and \register, \stack, #8
  sub \stack, \register
  push { \register, \additional }
.endm

// restore stack possibly changed due to stack alignment
.macro stack_unwind register:req, additional:req, stack:req
  pop { \register, \additional }
  add \stack, \register
.endm

// macro to switch to mode
.macro switch_mode mode:req, fixup:req
  // adjust lr
  sub lr, #\fixup
  // push return lr and spsr to modes stack
  srsdb sp!, #\mode
  // switch to given mode with interrupts disabled
  cpsid iaf, #\mode
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

.macro push_register name:req, mode:req, fixup:req
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
  stm lr, { r0 - r14 }^
  // wait for stm to populate all registers
  nop
  isb
  dsb
  // push fpu register
  push_fpu_register lr

  // get pushed lr and fill it into struct
  ldr r0, [ sp, #0 ]
  str r0, [ lr, #PC_OFFSET ]
  // get pushed spsr and fill it into struct
  ldr r0, [ sp, #4 ]
  str r0, [ lr, #SPSR_OFFSET ]

  // adjust stack pointer back
  add sp, #8

  // pass 0 as parameter to indicate user entry
  mov r1, #0

  // jump to finished
  b _\name\()_entry_finished

// privileged entry
_\name\()_entry_privileged:
  // allocate space for context structure
  sub sp, #STACK_FRAME_SIZE

  // save registers to stack
  stmia sp!, { r0 - r12 }
  sub sp, #SP_OFFSET

  // push fpu register
  push_fpu_register sp

  // get pushed lr and fill it into struct
  ldr r0, [ sp, #PRIVILEGED_PC_OFFSET ]
  str r0, [ sp, #PC_OFFSET ]

  // get pushed spsr and fill it into struct
  ldr r0, [ sp, #PRIVILEGED_SPSR_OFFSET ]
  str r0, [ sp, #SPSR_OFFSET ]

  // calculate previous stack pointer and push to context
  add r0, sp, #(STACK_FRAME_SIZE + 8)
  str r0, [ sp, #SP_OFFSET ]

  // populate lr
  str lr, [ sp, #LR_OFFSET ]

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
  mov sp, \stack
  // load old pc
  ldr r0, [ sp, #PC_OFFSET ]
  // push back onto stack
  str r0, [ sp, #PRIVILEGED_PC_OFFSET ]
  // load old pc
  ldr r0, [ sp, #SPSR_OFFSET ]
  // push back onto stack
  str r0, [ sp, #PRIVILEGED_SPSR_OFFSET ]
  // restore previous state
  pop_fpu_register sp
  // restore register without push back
  ldm sp, { r0 - r12 }
  // correct stack pointer by removing kernel stack frame
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
  push_register \name, \mode, \fixup

  push { r4 }
  mov r4, r1

  // remote debugging
  #if defined( REMOTE_DEBUG )
    .ifnc \nested, all
      // ensure stack alignment
      stack_wind r0, r1, sp
      // disable  breakpoints
      bl debug_breakpoint_disable
      // restore stack adjustments
      stack_unwind r0, r1, sp
    .endif
  #endif

  // ensure stack alignment
  stack_wind r0, r1, sp
  // call handler routine with context parameter
  mov r0, r4
  bl \handler
  // restore stack adjustments
  stack_unwind r0, r1, sp

  // ensure stack alignment
  stack_wind r0, r1, sp
  // enable interrupts
  .ifc \nested, all
    cpsie iaf
  .endif
  .ifc \nested, fast
    cpsie af
  .endif
  // handle events
  mov r0, r4
  bl event_handle
  // disable interrupts
  .ifc \nested, all
    cpsid iaf
  .endif
  .ifc \nested, fast
    cpsid af
  .endif
  // restore stack adjustments
  stack_unwind r0, r1, sp

  // remote debugging
  #if defined( REMOTE_DEBUG )
    .ifnc \nested, all
      // ensure stack alignment
      stack_wind r0, r1, sp
      // add breakpoints
      bl debug_breakpoint_enable
      // restore stack adjustments
      stack_unwind r0, r1, sp
    .endif
  #endif

  mov r1, r4
  pop { r4 }

  // restore registers for return
  pop_register \name, \mode, r1
  // return from exception
  rfeia sp!
.endm


.macro exception_handler_svc fixup:req, handler:req, mode:req, name:req, nested:req
  // subtraction is mandatory for extraction of svc number
  sub lr, #\fixup
  sub sp, #8
  str lr, [ sp, #0 ]
  mrs lr, spsr
  str lr, [ sp, #4 ]

  // push register
  push_register \name, \mode, \fixup

  push { r4 }
  mov r4, r1

  // remote debugging
  #if defined( REMOTE_DEBUG )
    .ifnc \nested, all
      // ensure stack alignment
      stack_wind r0, r1, sp
      // disable  breakpoints
      bl debug_breakpoint_disable
      // restore stack adjustments
      stack_unwind r0, r1, sp
    .endif
  #endif

  // ensure stack alignment
  stack_wind r0, r1, sp
  // call handler routine with context parameter
  mov r0, r4
  bl \handler
  // restore stack adjustments
  stack_unwind r0, r1, sp

  // ensure stack alignment
  stack_wind r0, r1, sp
  // enable interrupts
  .ifc \nested, all
    cpsie iaf
  .endif
  .ifc \nested, fast
    cpsie af
  .endif
  // handle events
  mov r0, r4
  bl event_handle
  // disable interrupts
  .ifc \nested, all
    cpsid iaf
  .endif
  .ifc \nested, fast
    cpsid af
  .endif
  // restore stack adjustments
  stack_unwind r0, r1, sp

  // remote debugging
  #if defined( REMOTE_DEBUG )
    .ifnc \nested, all
      // ensure stack alignment
      stack_wind r0, r1, sp
      // add breakpoints
      bl debug_breakpoint_enable
      // restore stack adjustments
      stack_unwind r0, r1, sp
    .endif
  #endif

  mov r1, r4
  pop { r4 }

  // restore registers for return
  pop_register \name, \mode, r1
  // return from exception
  rfeia sp!
.endm

.macro push_fpu_register register:req
  #if defined( ARM_CPU_HAS_NEON )
    // save floating point control status
    vmrs r4, fpscr
    str r4, [ \register, #GENERAL_PURPOSE_REGISTER_END ]
    // start with first fpu registers
    add \register, #NEON_REGISTER_START
    // backup fpu registers
    vstmia.64 \register!, { d0 - d15 }
    vstmia.64 \register!, { d16 - d31 }
    // restore passed register to point to begin
    sub \register, #STACK_FRAME_SIZE
  #endif
.endm

.macro pop_fpu_register register:req
  #if defined( ARM_CPU_HAS_NEON )
    // restore floating point control status
    ldr r4, [ \register, #GENERAL_PURPOSE_REGISTER_END ]
    vmsr fpscr, r4
    // start with first fpu registers
    add \register, #NEON_REGISTER_START
    // restore fpu registers
    vldmia.64 \register!, { d0 - d15 }
    vldmia.64 \register!, { d16 - d31 }
    // restore passed register to point to begin
    sub \register, #STACK_FRAME_SIZE
  #endif
.endm
