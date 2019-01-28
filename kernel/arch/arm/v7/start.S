
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include "arch/arm/v7/cpu.h"

.arch_extension sec
.arch_extension virt

.section .text

.global arch_start
arch_start:
  // Check for HYP mode
  mrs r0, cpsr
  eor r0, r0, #0x1A
  tst r0, #0x1F
  bic r0, r0, #0x1F // clear mode bits
  orr r0, r0, #( CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT ) // mask IRQ/FIQ bits and set SVC mode
  bne 2f // branch if not HYP mode
  orr r0, r0, #0x100 // mask Abort bit
  adr lr, 3f
  msr spsr_cxsf, r0
  msr ELR_hyp, lr
  eret
2: msr cpsr_c, r0
3:

  // setup irq mode stack
  msr cpsr_c, #( CPSR_MODE_IRQ | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_irq_mode
  // setup fiq mode stack
  msr cpsr_c, #( CPSR_MODE_FIQ | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_fiq_mode
  // setup abort mode stack
  msr cpsr_c, #( CPSR_MODE_ABORT | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_abort_mode
  // setup system mode stack
  msr cpsr_c, #( CPSR_MODE_SYSTEM | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_system_mode
  // setup undefined mode stack
  msr cpsr_c, #( CPSR_MODE_UNDEFINED | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_undefined_mode
  // setup hypervisor mode stack
  msr cpsr_c, #( CPSR_MODE_HYPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_hypervisor_mode
  // setup monitor mode stack
  msr cpsr_c, #( CPSR_MODE_MONITOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_monitor_mode
  // setup supervisor mode stack
  msr cpsr_c, #( CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT )
  ldr sp, =stack_supervisor_mode

  // branch and link to kernel main entry
  ldr r3, =kernel_main
  blx r3  // blx may switch to Thumb mode, depending on the target address

halt:
  wfe // equivalent of x86 HLT instruction
  b halt

// data section
.section .bss

// align stacks 32 bit
.balign 4

// irq mode stack
.space 8192
stack_irq_mode:

// fiq mode stack
.space 8192
stack_fiq_mode:

// abort mode stack
.space 8192
stack_abort_mode:

// undefined mode stack
.space 8192
stack_undefined_mode:

// system mode stack
.space 8192
stack_system_mode:

// supervisor mode stack
.space 8192
stack_supervisor_mode:

// hypervisor mode stack
.space 8192
stack_hypervisor_mode:

// monitor mode stack
.space 8192
stack_monitor_mode:
