
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

#define MAILBOX_BASE ( PERI_BASE + 0xB880 )
#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

.section .text.boot

.global boot_start
boot_start:
  // setup temporary stack
  ldr r4, =boot_start
  mov sp, r4

  // save boot parameter data
  push { r0 - r2 }

  // arch related boot start code
  bl boot_arch_start

  // clear bss section
  bl boot_bss_clear

  // prepare by vendor
  bl boot_vendor_prepare

  // restore boot parameter data
  pop { r0 - r2 }

  // load higher half start
  ldr r3, =start

  // set stack
  mov sp, r3

  // set program counter
  mov pc, r3
