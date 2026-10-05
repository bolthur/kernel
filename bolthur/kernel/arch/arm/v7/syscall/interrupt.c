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

#include "../cpu.h"
#include "../../../../syscall.h"
#if defined( PRINT_SYSCALL )
  #include "../../../../debug/debug.h"
#endif

/**
 * @fn void syscall_interrupt_handled(void*)
 * @brief Interrupt handled syscall
 * @param context
 */
void syscall_interrupt_handled( void* context ) {
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT(
      "syscall_interrupt_handled() from %d / %d\r\n",
      task_thread_current_thread->process->id, task_thread_current_thread->id
    )
  #endif
  // check for correct state for rpc end
  if ( ! task_thread_current_thread->handling_interrupt ) {
    // debug output
    #if defined( PRINT_SYSCALL )
      DEBUG_OUTPUT( "Not handling an interrupt\r\n" )
    #endif
    // populate success
    syscall_populate_success( context, 0 );
    // skip rest
    return;
  }
  // debug output
  #if defined( PRINT_SYSCALL )
    DEBUG_OUTPUT( "Disabling handling interrupt flag\r\n" )
  #endif
  // get register context
  cpu_register_context_t* cpu = task_thread_current_thread->current_context;
  // mask interrupt bits
  cpu->reg.spsr &= ~( CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT |  CPSR_ASYNC_ABORT_INHIBIT );
  // reset flag
  task_thread_current_thread->handling_interrupt = false;
  // populate success
  syscall_populate_success( context, 0 );
}
