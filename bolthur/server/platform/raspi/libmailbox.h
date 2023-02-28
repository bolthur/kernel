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

#if ! defined( _LIBMAILBOX_H )
#define _LIBMAILBOX_H

#define MAILBOX_GET_POWER_STATE 0x20001
#define MAILBOX_SET_POWER_STATE 0x28001
#define MAILBOX_GET_CLOCK_RATE 0x30002

// power state devices
#define MAILBOX_POWER_STATE_DEVICE_SD_CARD 0x0
#define MAILBOX_POWER_STATE_DEVICE_UART0 0x1
#define MAILBOX_POWER_STATE_DEVICE_UART1 0x2
#define MAILBOX_POWER_STATE_DEVICE_USB_HCD 0x3
#define MAILBOX_POWER_STATE_DEVICE_I2C0 0x4
#define MAILBOX_POWER_STATE_DEVICE_I2C1 0x5
#define MAILBOX_POWER_STATE_DEVICE_I2C2 0x6
#define MAILBOX_POWER_STATE_DEVICE_SPI 0x7
#define MAILBOX_POWER_STATE_DEVICE_CPP2TX 0x8
// get power state status bits
#define MAILBOX_GET_POWER_STATE_ON ( 1 << 0 )
#define MAILBOX_GET_POWER_STATE_DEVICE_NOT_EXISTING ( 1 << 1 )
// set power state status bits
#define MAILBOX_SET_POWER_STATE_ON ( 1 << 0 )
#define MAILBOX_SET_POWER_STATE_WAIT ( 1 << 1 )
// clock ids
#define MAILBOX_CLOCK_RESERVED 0
#define MAILBOX_CLOCK_EMMC 1
#define MAILBOX_CLOCK_UART 2
#define MAILBOX_CLOCK_ARM 3
#define MAILBOX_CLOCK_CORE 4
#define MAILBOX_CLOCK_V3D 5
#define MAILBOX_CLOCK_H264 6
#define MAILBOX_CLOCK_ISP 7
#define MAILBOX_CLOCK_SDRAM 8
#define MAILBOX_CLOCK_PIXEL 9
#define MAILBOX_CLOCK_PWM 10

#endif
