
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

#include "kernel/arch/arm/v7/cpu.h"
#include "kernel/arch/arm/stack.h"

.section .text

.global arch_start
arch_start:
  // setup irq mode stack
  msr cpsr_c, #( CPSR_MODE_IRQ | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_irq_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup fiq mode stack
  msr cpsr_c, #( CPSR_MODE_FIQ | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_fiq_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup abort mode stack
  msr cpsr_c, #( CPSR_MODE_ABORT | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_abort_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup system mode stack
  msr cpsr_c, #( CPSR_MODE_SYSTEM | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_system_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup undefined mode stack
  msr cpsr_c, #( CPSR_MODE_UNDEFINED | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_undefined_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup hypervisor mode stack
  msr cpsr_c, #( CPSR_MODE_HYPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_hypervisor_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup monitor mode stack
  msr cpsr_c, #( CPSR_MODE_MONITOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr r0, =stack_monitor_mode
  add r0, r0, #STACK_SIZE
  mov sp, r0

  // setup supervisor mode stack
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
