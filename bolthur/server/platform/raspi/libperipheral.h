/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
// dma
#define PERIPHERAL_DMA_OFFSET 0x7000
// dma channel 0
#define PERIPHERAL_DMA0_BASE PERIPHERAL_DMA_OFFSET
#define PERIPHERAL_DMA0_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA0_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA0_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA0_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA0_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA0_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA0_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA0_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA0_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 1
#define PERIPHERAL_DMA1_BASE PERIPHERAL_DMA_OFFSET + 0x100
#define PERIPHERAL_DMA1_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA1_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA1_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA1_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA1_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA1_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA1_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA1_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA1_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 2
#define PERIPHERAL_DMA2_BASE PERIPHERAL_DMA_OFFSET + 0x200
#define PERIPHERAL_DMA2_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA2_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA2_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA2_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA2_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA2_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA2_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA2_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA2_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 3
#define PERIPHERAL_DMA3_BASE PERIPHERAL_DMA_OFFSET + 0x300
#define PERIPHERAL_DMA3_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA3_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA3_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA3_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA3_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA3_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA3_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA3_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA3_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 4
#define PERIPHERAL_DMA4_BASE PERIPHERAL_DMA_OFFSET + 0x400
#define PERIPHERAL_DMA4_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA4_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA4_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA4_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA4_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA4_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA4_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA4_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA4_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 5
#define PERIPHERAL_DMA5_BASE PERIPHERAL_DMA_OFFSET + 0x500
#define PERIPHERAL_DMA5_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA5_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA5_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA5_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA5_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA5_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA5_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA5_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA5_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 6
#define PERIPHERAL_DMA6_BASE PERIPHERAL_DMA_OFFSET + 0x600
#define PERIPHERAL_DMA6_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA6_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA6_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA6_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA6_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA6_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA6_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA6_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA6_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 7
#define PERIPHERAL_DMA7_BASE PERIPHERAL_DMA_OFFSET + 0x700
#define PERIPHERAL_DMA7_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA7_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA7_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA7_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA7_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA7_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA7_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA7_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA7_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 8
#define PERIPHERAL_DMA8_BASE PERIPHERAL_DMA_OFFSET + 0x800
#define PERIPHERAL_DMA8_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA8_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA8_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA8_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA8_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA8_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA8_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA8_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA8_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 9
#define PERIPHERAL_DMA9_BASE PERIPHERAL_DMA_OFFSET + 0x900
#define PERIPHERAL_DMA9_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA9_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA9_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA9_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA9_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA9_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA9_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA9_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA9_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 10
#define PERIPHERAL_DMA10_BASE PERIPHERAL_DMA_OFFSET + 0xA00
#define PERIPHERAL_DMA10_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA10_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA10_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA10_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA10_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA10_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA10_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA10_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA10_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 11
#define PERIPHERAL_DMA11_BASE PERIPHERAL_DMA_OFFSET + 0xB00
#define PERIPHERAL_DMA11_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA11_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA11_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA11_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA11_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA11_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA11_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA11_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA11_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 12
#define PERIPHERAL_DMA12_BASE PERIPHERAL_DMA_OFFSET + 0xC00
#define PERIPHERAL_DMA12_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA12_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA12_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA12_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA12_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA12_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA12_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA12_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA12_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 13
#define PERIPHERAL_DMA13_BASE PERIPHERAL_DMA_OFFSET + 0xD00
#define PERIPHERAL_DMA13_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA13_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA13_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA13_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA13_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA13_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA13_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA13_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA13_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// dma channel 14
#define PERIPHERAL_DMA14_BASE PERIPHERAL_DMA_OFFSET + 0xE00
#define PERIPHERAL_DMA14_CS PERIPHERAL_DMA_OFFSET + 0x0
#define PERIPHERAL_DMA14_CONBLK_AD PERIPHERAL_DMA_OFFSET + 0x4
#define PERIPHERAL_DMA14_TI PERIPHERAL_DMA_OFFSET + 0x8
#define PERIPHERAL_DMA14_SOURCE_AD PERIPHERAL_DMA_OFFSET + 0xC
#define PERIPHERAL_DMA14_DEST_AD PERIPHERAL_DMA_OFFSET + 0x10
#define PERIPHERAL_DMA14_TXFR_LEN PERIPHERAL_DMA_OFFSET + 0x14
#define PERIPHERAL_DMA14_STRIDE PERIPHERAL_DMA_OFFSET + 0x18
#define PERIPHERAL_DMA14_NEXTCONBK PERIPHERAL_DMA_OFFSET + 0x1C
#define PERIPHERAL_DMA14_DEBUG PERIPHERAL_DMA_OFFSET + 0x20
// interrupt status and enable register
#define PERIPHERAL_DMA_INT_STATUS PERIPHERAL_DMA_OFFSET + 0xFE0
#define PERIPHERAL_DMA_ENABLE PERIPHERAL_DMA_OFFSET + 0xFF0
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
#define PERIPHERAL_SDHOST_HOST_CONFIG PERIPHERAL_SDHOST_OFFSET + 0x38
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
