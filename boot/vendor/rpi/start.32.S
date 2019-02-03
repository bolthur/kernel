
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

// GPIO
#define GPIO_BASE ( PERI_BASE + 0x200000 )
#define GPFSEL0 ( GPIO_BASE + 0 ) // function select 0
#define GPFSEL1 ( GPIO_BASE + 0x04 ) // function select 1
#define GPFSEL2 ( GPIO_BASE + 0x08 ) // function select 2
#define GPFSEL3 ( GPIO_BASE + 0x0C ) // function select 3
#define GPFSEL4 ( GPIO_BASE + 0x10 ) // function select 4
#define GPFSEL5 ( GPIO_BASE + 0x14 ) // function select 5
#define GPSET0 ( GPIO_BASE + 0x1C ) // pin output set 0
#define GPSET1 ( GPIO_BASE + 0x20 ) // pin output set 1
#define GPCLR0 ( GPIO_BASE + 0x28 ) // pin output clear 0
#define GPCLR1 ( GPIO_BASE + 0x2C ) // pin output clear 1
#define GPLEV0 ( GPIO_BASE + 0x34 ) // pin level 0
#define GPLEV1 ( GPIO_BASE + 0x38 ) // pin level 1
#define GPEDS0 ( GPIO_BASE + 0x40 ) // pin event detect status 0
#define GPEDS1 ( GPIO_BASE + 0x44 ) // pin event detect status 1
#define GPREN0 ( GPIO_BASE + 0x4C ) // pin rising edge detect enable 0
#define GPREN1 ( GPIO_BASE + 0x50 ) // pin rising edge detect enable 1
#define GPFEN0 ( GPIO_BASE + 0x58 ) // pin falling edge detect enable 0
#define GPFEN1 ( GPIO_BASE + 0x5C ) // pin falling edge detect enable 1
#define GPHEN0 ( GPIO_BASE + 0x64 ) // pin high detect enable 0
#define GPHEN1 ( GPIO_BASE + 0x68 ) // pin high detect enable 1
#define GPLEN0 ( GPIO_BASE + 0x70 ) // pin low detect enable 0
#define GPLEN1 ( GPIO_BASE + 0x74 ) // pin low detect enable 1
#define GPAREN0 ( GPIO_BASE + 0x7C ) // pin async rising edge detect enable 0
#define GPAREN1 ( GPIO_BASE + 0x80 ) // pin async rising edge detect enable 1
#define GPAFEN0 ( GPIO_BASE + 0x88 ) // pin async falling edge detect enable 0
#define GPAFEN1 ( GPIO_BASE + 0x8C ) // pin async falling edge detect enable 1
#define GPPUD ( GPIO_BASE + 0x94 ) // pin pull up/down enable
#define GPPUDCLK0 ( GPIO_BASE + 0x98 ) // pin pull up/down enable clock 0
#define GPPUDCLK1 ( GPIO_BASE + 0x9C ) // pin pull up/down enable clock 1

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
  ldr r4, =start
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

  // Disable uart 0
  ldr r0, =UART0_CR
  mov r1, #0
  bl put32

  // Setup the GPIO pin 14 && 15
  ldr r0, =GPFSEL1
  bl get32
  and r0, r0, #( ~( 7 << 12 ) )
  orr r0, r0, #( ( 4 << 12 ) )
  and r0, r0, #( ~( 7 << 15 ) )
  orr r0, r0, #( ( 4 << 15 ) )
  mov r1, r0
  ldr r0, =GPFSEL1
  bl put32

  // Disable pull up/down for all GPIO pins & delay for 150 cycles.
  ldr r0, =GPPUD
  mov r1, #0
  bl put32

  // delay 150 cycles
  movs  r0, #150
5:
  subs r0, r0, #1
  bne 5b

  // Disable pull up/down for pin 14,15 & delay for 150 cycles.
  ldr r0, =GPPUDCLK0
  mov r1, #( ( 1 << 14 ) | ( 1 << 15 ) )
  bl put32

  // delay 150 cycles
  movs  r0, #150
6:
  subs r0, r0, #1
  bne 6b

  // Write 0 to GPPUDCLK0 to make it take effect.
  ldr r0, =GPPUDCLK0
  mov r1, #0
  bl put32

  // Clear pending interrupts.
  ldr r0, =UART0_ICR
  mov r1, #0x7FF
  bl put32

  // Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
  ldr r0, =UART0_IBRD
  mov r1, #1
  bl put32

  // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
  ldr r0, =UART0_FBRD
  mov r1, #40
  bl put32

  // Enable FIFO & 8 bit data transmissio (1 stop bit no parity).
  ldr r0, =UART0_LCRH
  mov r1, #( ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) )
  bl put32

  // Mask all interrupts.
  ldr r0, =UART0_IMSC
  mov r1, #( ( 1 << 1 ) | ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 ) | ( 1 << 7 ) | ( 1 << 8 ) | ( 1 << 9 ) | ( 1 << 10 ) )
  bl put32

  // Enable UART0 receive & transfer part of UART.
  ldr r0, =UART0_CR
  mov r1, #( ( 1 << 0 ) | ( 1 << 8 ) | ( 1 << 9 ) )
  bl put32

7:
  // wait until ready
  ldr r0, =UART0_FR
  bl get32
  ands r0, #( 1 << 5 )
  bne 7b

  // write character
  ldr r0, =UART0_DR
  mov r1, #( 'c' )
  bl put32

halt:
  wfe // equivalent of x86 HLT instruction
  b halt

put32:
  str r1, [ r0 ]
  bx lr

get32:
  ldr r0, [ r0 ]
  bx lr
