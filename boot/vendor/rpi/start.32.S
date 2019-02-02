
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

// Testing defines
#define PERI_BASE    0x3F000000
#define UART0_BASE   (PERI_BASE+0x201000)
#define UART0_DR     (UART0_BASE+0x00)
#define UART0_RSRECR (UART0_BASE+0x04)
#define UART0_FR     (UART0_BASE+0x18)
#define UART0_ILPR   (UART0_BASE+0x20)
#define UART0_IBRD   (UART0_BASE+0x24)
#define UART0_FBRD   (UART0_BASE+0x28)
#define UART0_LCRH   (UART0_BASE+0x2C)
#define UART0_CR     (UART0_BASE+0x30)
#define UART0_IFLS   (UART0_BASE+0x34)
#define UART0_IMSC   (UART0_BASE+0x38)
#define UART0_RIS    (UART0_BASE+0x3C)
#define UART0_MIS    (UART0_BASE+0x40)
#define UART0_ICR    (UART0_BASE+0x44)
#define UART0_DMACR  (UART0_BASE+0x48)
#define UART0_ITCR   (UART0_BASE+0x80)
#define UART0_ITIP   (UART0_BASE+0x84)
#define UART0_ITOP   (UART0_BASE+0x88)
#define UART0_TDR    (UART0_BASE+0x8C)

#define CPSR_MODE_USER 0x10
#define CPSR_MODE_FIQ 0x11
#define CPSR_MODE_IRQ 0x12
#define CPSR_MODE_SUPERVISOR 0x13
#define CPSR_MODE_MONITOR 0x16
#define CPSR_MODE_ABORT 0x17
#define CPSR_MODE_HYPERVISOR  0x1A
#define CPSR_MODE_UNDEFINED 0x1B
#define CPSR_MODE_SYSTEM 0x1F

#define CPSR_IRQ_INHIBIT 0x80
#define CPSR_FIQ_INHIBIT 0x40
#define CPSR_THUMB 0x20

#define STACK_FRAME_SIZE 68

#define SYS_CTRL_REG_ENABLE_DATA_CACHE 0x1 << 2
#define SYS_CTRL_REG_ENABLE_BRANCH_PREDICTION 0x1 << 11
#define SYS_CTRL_REG_ENABLE_INSTRUCTION_CACHE 0x1 << 12

#include "kernel/entry.h"

.section .text.boot

.arch_extension sec
.arch_extension virt

.global start
start:
  // setup temporary stack
  ldr r4, =start - KERNEL_OFFSET
  mov sp, r4

  // Return current CPU ID (0..3)
  cpsid if // Disable IRQ & FIQ
  mrc p15, 0, r3, c0, c0, 5 // r3 = Multiprocessor Affinity Register (MPIDR)
  ands r3, #3 // r0 = CPU ID (Bits 0..1)
  bne 1f // If (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)
  beq 2f // If equal branch to normal startup

1:
  wfe
  b 1
2:

  // check for hypervisor mode and switch back to supervisor mode for rpi2 and rpi3
  mrs r0, cpsr
  eor r0, r0, #0x1A
  tst r0, #0x1F
  bic r0, r0, #0x1F // clear mode bits
  orr r0, r0, #( CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT ) // mask IRQ/FIQ bits and set SVC mode
  bne 3f // branch if not HYP mode
  orr r0, r0, #0x100 // mask Abort bit
  adr lr, 4f
  msr spsr_cxsf, r0
  msr ELR_hyp, lr
  eret
3: msr cpsr_c, r0
4:

halt:
  wfe // equivalent of x86 HLT instruction
  b halt
