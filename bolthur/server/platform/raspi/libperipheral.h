/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#if ! defined( _LIBPERIPHERAL_H )
#define _LIBPERIPHERAL_H

// mailbox offset
#define PERIPHERAL_MAILBOX_OFFSET 0xB880
// random number generator
#define PERIPHERAL_RNG_OFFSET 0x104000
#define PERIPHERAL_RNG_CONTROL PERIPHERAL_RNG_OFFSET + 0x00
#define PERIPHERAL_RNG_STATUS PERIPHERAL_RNG_OFFSET + 0x04
#define PERIPHERAL_RNG_DATA PERIPHERAL_RNG_OFFSET + 0x08
#define PERIPHERAL_RNG_INTERRUPT PERIPHERAL_RNG_OFFSET + 0x10
// gpio
#define PERIPHERAL_GPIO_OFFSET 0x200000
#define PERIPHERAL_GPIO_GPFSEL0 PERIPHERAL_GPIO_OFFSET
#define PERIPHERAL_GPIO_GPFSEL1 PERIPHERAL_GPIO_OFFSET + 0x04
#define PERIPHERAL_GPIO_GPFSEL2 PERIPHERAL_GPIO_OFFSET + 0x08
#define PERIPHERAL_GPIO_GPFSEL3 PERIPHERAL_GPIO_OFFSET + 0x0C
#define PERIPHERAL_GPIO_GPFSEL4 PERIPHERAL_GPIO_OFFSET + 0x10
#define PERIPHERAL_GPIO_GPFSEL5 PERIPHERAL_GPIO_OFFSET + 0x14
#define PERIPHERAL_GPIO_GPSET0 PERIPHERAL_GPIO_OFFSET + 0x1C
#define PERIPHERAL_GPIO_GPSET1 PERIPHERAL_GPIO_OFFSET + 0x20
#define PERIPHERAL_GPIO_GPCLR0 PERIPHERAL_GPIO_OFFSET + 0x28
#define PERIPHERAL_GPIO_GPCLR1 PERIPHERAL_GPIO_OFFSET + 0x2C
#define PERIPHERAL_GPIO_GPLEV0 PERIPHERAL_GPIO_OFFSET + 0x34
#define PERIPHERAL_GPIO_GPLEV1 PERIPHERAL_GPIO_OFFSET + 0x38
#define PERIPHERAL_GPIO_GPEDS0 PERIPHERAL_GPIO_OFFSET + 0x40
#define PERIPHERAL_GPIO_GPEDS1 PERIPHERAL_GPIO_OFFSET + 0x44
#define PERIPHERAL_GPIO_GPREN0 PERIPHERAL_GPIO_OFFSET + 0x4C
#define PERIPHERAL_GPIO_GPREN1 PERIPHERAL_GPIO_OFFSET + 0x50
#define PERIPHERAL_GPIO_GPFEN0 PERIPHERAL_GPIO_OFFSET + 0x58
#define PERIPHERAL_GPIO_GPFEN1 PERIPHERAL_GPIO_OFFSET + 0x5C
#define PERIPHERAL_GPIO_GPHEN0 PERIPHERAL_GPIO_OFFSET + 0x64
#define PERIPHERAL_GPIO_GPHEN1 PERIPHERAL_GPIO_OFFSET + 0x68
#define PERIPHERAL_GPIO_GPLEN0 PERIPHERAL_GPIO_OFFSET + 0x70
#define PERIPHERAL_GPIO_GPLEN1 PERIPHERAL_GPIO_OFFSET + 0x74
#define PERIPHERAL_GPIO_GPAREN0 PERIPHERAL_GPIO_OFFSET + 0x7C
#define PERIPHERAL_GPIO_GPAREN1 PERIPHERAL_GPIO_OFFSET + 0x80
#define PERIPHERAL_GPIO_GPAFEN0 PERIPHERAL_GPIO_OFFSET + 0x88
#define PERIPHERAL_GPIO_GPAFEN1 PERIPHERAL_GPIO_OFFSET + 0x8C
#define PERIPHERAL_GPIO_GPPUD PERIPHERAL_GPIO_OFFSET + 0x94
#define PERIPHERAL_GPIO_GPPUDCLK0 PERIPHERAL_GPIO_OFFSET + 0x98
#define PERIPHERAL_GPIO_GPPUDCLK1 PERIPHERAL_GPIO_OFFSET + 0x9C
// sdhost
#define PERIPHERAL_SDHOST_OFFSET 0x202000
#define PERIPHERAL_SDHOST_COMMAND PERIPHERAL_SDHOST_OFFSET
#define PERIPHERAL_SDHOST_ARGUMENT PERIPHERAL_SDHOST_OFFSET + 0x04
#define PERIPHERAL_SDHOST_TIMEOUTCOUNTER PERIPHERAL_SDHOST_OFFSET + 0x08
#define PERIPHERAL_SDHOST_CLOCKDIVISOR PERIPHERAL_SDHOST_OFFSET + 0x0C
#define PERIPHERAL_SDHOST_RESPONSE0 PERIPHERAL_SDHOST_OFFSET + 0x10
#define PERIPHERAL_SDHOST_RESPONSE1 PERIPHERAL_SDHOST_OFFSET + 0x14
#define PERIPHERAL_SDHOST_RESPONSE2 PERIPHERAL_SDHOST_OFFSET + 0x18
#define PERIPHERAL_SDHOST_RESPONSE3 PERIPHERAL_SDHOST_OFFSET + 0x1C
#define PERIPHERAL_SDHOST_HOST_STATUS PERIPHERAL_SDHOST_OFFSET + 0x20
#define PERIPHERAL_SDHOST_POWER PERIPHERAL_SDHOST_OFFSET + 0x30 // VDD?
#define PERIPHERAL_SDHOST_DEBUG PERIPHERAL_SDHOST_OFFSET + 0x34 // EDM?
#define PERIPHERAL_SDHOST_HOSTCONFIG PERIPHERAL_SDHOST_OFFSET + 0x38
#define PERIPHERAL_SDHOST_BLOCKSIZE PERIPHERAL_SDHOST_OFFSET + 0x3C
#define PERIPHERAL_SDHOST_DATAPORT PERIPHERAL_SDHOST_OFFSET + 0x40
#define PERIPHERAL_SDHOST_BLOCKCOUNT PERIPHERAL_SDHOST_OFFSET + 0x50
// emmc
#define PERIPHERAL_EMMC_OFFSET 0x300000
#define PERIPHERAL_EMMC_ARG2 PERIPHERAL_EMMC_OFFSET // ACMD23 argument
#define PERIPHERAL_EMMC_BLKSIZECNT PERIPHERAL_EMMC_OFFSET + 0x04 // block and size count
#define PERIPHERAL_EMMC_ARG1 PERIPHERAL_EMMC_OFFSET + 0x08 // argument
#define PERIPHERAL_EMMC_CMDTM PERIPHERAL_EMMC_OFFSET + 0x0C // command and transfer mode
#define PERIPHERAL_EMMC_RESP0 PERIPHERAL_EMMC_OFFSET + 0x10 // response bits 31 : 0
#define PERIPHERAL_EMMC_RESP1 PERIPHERAL_EMMC_OFFSET + 0x14 // response bits 63 : 32
#define PERIPHERAL_EMMC_RESP2 PERIPHERAL_EMMC_OFFSET + 0x18 // response bits 95 : 64
#define PERIPHERAL_EMMC_RESP3 PERIPHERAL_EMMC_OFFSET + 0x1C // response bits 127 : 96
#define PERIPHERAL_EMMC_DATA PERIPHERAL_EMMC_OFFSET + 0x20 // data
#define PERIPHERAL_EMMC_STATUS PERIPHERAL_EMMC_OFFSET + 0x24 // status
#define PERIPHERAL_EMMC_CONTROL0 PERIPHERAL_EMMC_OFFSET + 0x28 // host configuration bits
#define PERIPHERAL_EMMC_CONTROL1 PERIPHERAL_EMMC_OFFSET + 0x2C // host configuration bits
#define PERIPHERAL_EMMC_INTERRUPT PERIPHERAL_EMMC_OFFSET + 0x30 // interrupt flags
#define PERIPHERAL_EMMC_IRPT_MASK PERIPHERAL_EMMC_OFFSET + 0x34 // interrupt flag enable
#define PERIPHERAL_EMMC_IRPT_ENABLE PERIPHERAL_EMMC_OFFSET + 0x38 // interrupt generation enable
#define PERIPHERAL_EMMC_CONTROL2 PERIPHERAL_EMMC_OFFSET + 0x3C // host configuration bits
#define PERIPHERAL_EMMC_FORCE_IRPT PERIPHERAL_EMMC_OFFSET + 0x50 // force interrupt event
#define PERIPHERAL_EMMC_BOOT_TIMEOUT PERIPHERAL_EMMC_OFFSET + 0x70 // timeout in boot mode
#define PERIPHERAL_EMMC_DBG_SEL PERIPHERAL_EMMC_OFFSET + 0x74 // debug bus configuration
#define PERIPHERAL_EMMC_EXRDFIFO_CFG PERIPHERAL_EMMC_OFFSET + 0x80 // extension fifo configuration
#define PERIPHERAL_EMMC_EXRDFIFO_EN PERIPHERAL_EMMC_OFFSET + 0x84 // extension fifo enable
#define PERIPHERAL_EMMC_TUNE_STEP PERIPHERAL_EMMC_OFFSET + 0x88 // delay per card clock tuning step
#define PERIPHERAL_EMMC_TUNE_STEPS_STD PERIPHERAL_EMMC_OFFSET + 0x8C // card clock tuning steps for SDR
#define PERIPHERAL_EMMC_TUNE_STEPS_DDR PERIPHERAL_EMMC_OFFSET + 0x90 // card clock tuning steps for DDR
#define PERIPHERAL_EMMC_SPI_INT_SPT PERIPHERAL_EMMC_OFFSET + 0xF0 // spi interrupt support
#define PERIPHERAL_EMMC_SLOTISR_VER PERIPHERAL_EMMC_OFFSET + 0xFC // slot interrupt status and version
// usb
#define PERIPHERAL_USB_OFFSET 0x980000

#endif
