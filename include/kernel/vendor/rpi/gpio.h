
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

#if ! defined( __KERNEL_VENDOR_RPI_GPIO__ )
#define __KERNEL_VENDOR_RPI_GPIO__

#if defined( BCM2709 ) || defined( BCM2710 )
  #define CORE0_TIMER_IRQCNTL 0x40000040
  #define CORE0_IRQ_SOURCE 0x40000060
#endif

enum {
  GPIO_BASE = 0x00200000,

  // GPIO
  GPFSEL0 = ( GPIO_BASE + 0 ), // function select 0
  GPFSEL1 = ( GPIO_BASE + 0x04 ), // function select 1
  GPFSEL2 = ( GPIO_BASE + 0x08 ), // function select 2
  GPFSEL3 = ( GPIO_BASE + 0x0C ), // function select 3
  GPFSEL4 = ( GPIO_BASE + 0x10 ), // function select 4
  GPFSEL5 = ( GPIO_BASE + 0x14 ), // function select 5
  GPSET0 = ( GPIO_BASE + 0x1C ), // pin output set 0
  GPSET1 = ( GPIO_BASE + 0x20 ), // pin output set 1
  GPCLR0 = ( GPIO_BASE + 0x28 ), // pin output clear 0
  GPCLR1 = ( GPIO_BASE + 0x2C ), // pin output clear 1
  GPLEV0 = ( GPIO_BASE + 0x34 ), // pin level 0
  GPLEV1 = ( GPIO_BASE + 0x38 ), // pin level 1
  GPEDS0 = ( GPIO_BASE + 0x40 ), // pin event detect status 0
  GPEDS1 = ( GPIO_BASE + 0x44 ), // pin event detect status 1
  GPREN0 = ( GPIO_BASE + 0x4C ), // pin rising edge detect enable 0
  GPREN1 = ( GPIO_BASE + 0x50 ), // pin rising edge detect enable 1
  GPFEN0 = ( GPIO_BASE + 0x58 ), // pin falling edge detect enable 0
  GPFEN1 = ( GPIO_BASE + 0x5C ), // pin falling edge detect enable 1
  GPHEN0 = ( GPIO_BASE + 0x64 ), // pin high detect enable 0
  GPHEN1 = ( GPIO_BASE + 0x68 ), // pin high detect enable 1
  GPLEN0 = ( GPIO_BASE + 0x70 ), // pin low detect enable 0
  GPLEN1 = ( GPIO_BASE + 0x74 ), // pin low detect enable 1
  GPAREN0 = ( GPIO_BASE + 0x7C ), // pin async rising edge detect enable 0
  GPAREN1 = ( GPIO_BASE + 0x80 ), // pin async rising edge detect enable 1
  GPAFEN0 = ( GPIO_BASE + 0x88 ), // pin async falling edge detect enable 0
  GPAFEN1 = ( GPIO_BASE + 0x8C ), // pin async falling edge detect enable 1
  GPPUD = ( GPIO_BASE + 0x94 ), // pin pull up/down enable
  GPPUDCLK0 = ( GPIO_BASE + 0x98 ), // pin pull up/down enable clock 0
  GPPUDCLK1 = ( GPIO_BASE + 0x9C ), // pin pull up/down enable clock 1

  // auxiliary
  AUX_BASE = ( GPIO_BASE + 0x15000 ),
  AUX_IRQ = ( AUX_BASE + 0x00 ), // interrupt status
  AUX_ENABLES = ( AUX_BASE + 0x04 ), // enables

  // mini UART
  AUX_MU_IO_REG = ( AUX_BASE + 0x40 ), // I/O data
  AUX_MU_IER_REG = ( AUX_BASE + 0x44 ), // interrupt enable
  AUX_MU_IIR_REG = ( AUX_BASE + 0x48 ), // interrupt identify
  AUX_MU_LCR_REG = ( AUX_BASE + 0x4C ), // line control
  AUX_MU_MCR_REG = ( AUX_BASE + 0x50 ), // modem control
  AUX_MU_LSR_REG = ( AUX_BASE + 0x54 ), // line status
  AUX_MU_MSR_REG = ( AUX_BASE + 0x58 ), // modem status
  AUX_MU_SCRATCH = ( AUX_BASE + 0x5C ), // scratch
  AUX_MU_CNTL_REG = ( AUX_BASE + 0x60 ), // extra control
  AUX_MU_STAT_REG = ( AUX_BASE + 0x64 ), // extra status
  AUX_MU_BAUD_REG = ( AUX_BASE + 0x68 ), // uart baud rate

  // SPI 1
  AUX_SPI0_CNTL0_REG = ( AUX_BASE + 0x80 ), // control register 0
  AUX_SPI0_CNTL1_REG = ( AUX_BASE + 0x84 ), // control register 1
  AUX_SPI0_STAT_REG = ( AUX_BASE + 0x88 ), // status
  AUX_SPI0_IO_REG = ( AUX_BASE + 0x90 ), // data
  AUX_SPI0_PEEK_REG = ( AUX_BASE + 0x94 ),  // peek

  // SPI 2
  AUX_SPI1_CNTL0_REG = ( AUX_BASE + 0xC0 ), // control register 0
  AUX_SPI1_CNTL1_REG = ( AUX_BASE + 0xC4 ), // control register 1
  AUX_SPI1_STAT_REG = ( AUX_BASE + 0xC8 ), // status
  AUX_SPI1_IO_REG = ( AUX_BASE + 0xD0 ), // data
  AUX_SPI1_PEEK_REG = ( AUX_BASE + 0xD4 ), // peek

  // UART0 ( PL011 )
  UART_BASE = ( GPIO_BASE + 0x1000 ),
  UARTDR = ( UART_BASE + 0x00 ), // data register
  UARTRSR = ( UART_BASE + 0x04 ), // receive status register /
  UARTECR = ( UART_BASE + 0x04 ), // error clear register
  UARTFR = ( UART_BASE + 0x18 ), // flag register
  UARTILPR = ( UART_BASE + 0x20 ), // low power container register
  UARTIBRD = ( UART_BASE + 0x24 ), // integer baud rate register
  UARTFBRD = ( UART_BASE + 0x28 ), // fractional baud rate register
  UARTLCRH = ( UART_BASE + 0x2C ), // line control register
  UARTCR = ( UART_BASE + 0x30 ), // control register
  UARTIFLS = ( UART_BASE + 0x34 ), // interrupt fifo level select register
  UARTIMSC = ( UART_BASE + 0x38 ), // interupt mask set/clear register
  UARTRIS = ( UART_BASE + 0x3C ), // raw interrupt status register
  UARTMIS = ( UART_BASE + 0x40 ), // masked interrupt status register
  UARTICR = ( UART_BASE + 0x44 ), // interrupt clear register
  UARTDMACR = ( UART_BASE + 0x48 ), // dma controll register

  // Interrupt register
  INTERRUPT_IRQ_BASE = 0xB200,
  INTERRUPT_IRQ_BASIC_PENDING = ( INTERRUPT_IRQ_BASE + 0x00 ),
  INTERRUPT_IRQ_PENDING_1 = ( INTERRUPT_IRQ_BASE + 0x04 ),
  INTERRUPT_IRQ_PENDING_2 = ( INTERRUPT_IRQ_BASE + 0x08 ),
  INTERRUPT_FIQ_CONTROL = ( INTERRUPT_IRQ_BASE + 0x0C ),
  INTERRUPT_ENABLE_IRQ_1 = ( INTERRUPT_IRQ_BASE + 0x10 ),
  INTERRUPT_ENABLE_IRQ_2 = ( INTERRUPT_IRQ_BASE + 0x14 ),
  INTERRUPT_ENABLE_IRQ_BASIC = ( INTERRUPT_IRQ_BASE + 0x18 ),
  INTERRUPT_DISABLE_IRQ_1 = ( INTERRUPT_IRQ_BASE + 0x1C ),
  INTERRUPT_DISABLE_IRQ_2 = ( INTERRUPT_IRQ_BASE + 0x20 ),
  INTERRUPT_DISABLE_IRQ_BASIC = ( INTERRUPT_IRQ_BASE + 0x24 ),

  // Arm timer
  ARM_TIMER_BASE = 0xB400,
  ARM_TIMER_LOAD = ( ARM_TIMER_BASE + 0x00 ),
  ARM_TIMER_VALUE = ( ARM_TIMER_BASE + 0x04 ),
  ARM_TIMER_CONTROL = ( ARM_TIMER_BASE + 0x08 ),
  ARM_TIMER_IRQ_CLEAR_ACK = ( ARM_TIMER_BASE + 0x0C ),
  ARM_TIMER_RAW_IRQ = ( ARM_TIMER_BASE + 0x10 ),
  ARM_TIMER_MASKED_IRQ = ( ARM_TIMER_BASE + 0x14 ),
  ARM_TIMER_RELOAD = ( ARM_TIMER_BASE + 0x18 ),
  ARM_TIMER_PRE_DIVIDER = ( ARM_TIMER_BASE + 0x1C ),
  ARM_TIMER_FREE_COUNTER = ( ARM_TIMER_BASE + 0x20 ),

  // System  timer
  SYSTEM_TIMER_BASE = 0x3000,
  SYSTEM_TIMER_CONTROL = ( SYSTEM_TIMER_BASE + 0x00 ),
  SYSTEM_TIMER_COUNTER_LOWER = ( SYSTEM_TIMER_BASE + 0x04 ),
  SYSTEM_TIMER_COUNTER_HIGHER = ( SYSTEM_TIMER_BASE + 0x08 ),
  SYSTEM_TIMER_COMPARE_0 = ( SYSTEM_TIMER_BASE + 0x0C ),
  SYSTEM_TIMER_COMPARE_1 = ( SYSTEM_TIMER_BASE + 0x10 ),
  SYSTEM_TIMER_COMPARE_2 = ( SYSTEM_TIMER_BASE + 0x14 ),
  SYSTEM_TIMER_COMPARE_3 = ( SYSTEM_TIMER_BASE + 0x18 ),
};

#endif
