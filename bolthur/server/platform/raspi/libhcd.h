/**
* Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _LIBHCD_H
#define _LIBHCD_H

// interrupt
#define ARM_IRQ_USB 9

// core interrupts
#define HCD_CORE_INTERRUPT_CURRENT_MODE ( 1 << 0 )
#define HCD_CORE_INTERRUPT_MODE_MISMATCH ( 1 << 1 )
#define HCD_CORE_INTERRUPT_OTG ( 1 << 2 )
#define HCD_CORE_INTERRUPT_DMA_START_OF_FRAME ( 1 << 3 )
#define HCD_CORE_INTERRUPT_RECEIVE_STATUS_LEVEL ( 1 << 4 )
#define HCD_CORE_INTERRUPT_NP_TRANSMIT_FIFO_EMPTY ( 1 << 5 )
#define HCD_CORE_INTERRUPT_GINNAKEFF ( 1 << 6 )
#define HCD_CORE_INTERRUPT_GOUTNAKEFF ( 1 << 7 )
#define HCD_CORE_INTERRUPT_ULPICK ( 1 << 8 )
#define HCD_CORE_INTERRUPT_I2C ( 1 << 9 )
#define HCD_CORE_INTERRUPT_EARLY_SUSPEND ( 1 << 10 )
#define HCD_CORE_INTERRUPT_USB_SUSPEND ( 1 << 11 )
#define HCD_CORE_INTERRUPT_USB_RESET ( 1 << 12 )
#define HCD_CORE_INTERRUPT_ENUMARTION_DONE ( 1 << 13 )
#define HCD_CORE_INTERRUPT_ISOCHRONOUS_OUT_DROP ( 1 << 14 )
#define HCD_CORE_INTERRUPT_EOPFRAME ( 1 << 15 )
#define HCD_CORE_INTERRUPT_RESTORE_DONE ( 1 << 16 )
#define HCD_CORE_INTERRUPT_END_POINT_MISMATCH ( 1 << 17 )
#define HCD_CORE_INTERRUPT_IN_END_POINT ( 1 << 18 )
#define HCD_CORE_INTERRUPT_OUT_END_POINT ( 1 << 19 )
#define HCD_CORE_INTERRUPT_INCOMPLETE_ISOCHRONOUS_IN ( 1 << 20 )
#define HCD_CORE_INTERRUPT_INCOMPLETE_ISOCHRONOUS_OUT ( 1 << 21 )
#define HCD_CORE_INTERRUPT_FETSETUP ( 1 << 22 )
#define HCD_CORE_INTERRUPT_RESET_DETECT ( 1 << 23 )
#define HCD_CORE_INTERRUPT_PORT ( 1 << 24 )
#define HCD_CORE_INTERRUPT_HOST_CHANNEL ( 1 << 25 )
#define HCD_CORE_INTERRUPT_HP_TRANSMIT_FIFO_EMPTY ( 1 << 26 )
#define HCD_CORE_INTERRUPT_LOW_POWER_MODE_TRANSMIT_RECEIVED ( 1 << 27 )
#define HCD_CORE_INTERRUPT_CONNECTION_ID_STATUS_CHANGE ( 1 << 28 )
#define HCD_CORE_INTERRUPT_DISCONNECT ( 1 << 29 )
#define HCD_CORE_INTERRUPT_SESSION_REQUEST ( 1 << 30 )
#define HCD_CORE_INTERRUPT_WAKEUP ( 1 << 31 )

// channel interrupts
#define HCD_CHANNEL_INTERRUPT_TRANSFER_COMPLETE ( 1 << 0 )
#define HCD_CHANNEL_INTERRUPT_HALT ( 1 << 1 )
#define HCD_CHANNEL_INTERRUPT_AHB_ERROR ( 1 << 2 )
#define HCD_CHANNEL_INTERRUPT_STALL ( 1 << 3 )
#define HCD_CHANNEL_INTERRUPT_NEGATIVE_ACKNOWLEDGEMENT ( 1 << 4 )
#define HCD_CHANNEL_INTERRUPT_ACKNOWLEDGEMENT ( 1 << 5 )
#define HCD_CHANNEL_INTERRUPT_NOT_YET ( 1 << 6 )
#define HCD_CHANNEL_INTERRUPT_TRANSACTION_ERROR ( 1 << 7 )
#define HCD_CHANNEL_INTERRUPT_BABBLE_ERROR ( 1 << 8 )
#define HCD_CHANNEL_INTERRUPT_FRAME_OVERRUN ( 1 << 9 )
#define HCD_CHANNEL_INTERRUPT_DATA_TOGGLE_ERROR ( 1 << 10 )
#define HCD_CHANNEL_INTERRUPT_BUFFER_NOT_AVAILABLE ( 1 << 11 )
#define HCD_CHANNEL_INTERRUPT_EXCESSIVE_TRANSMISSION ( 1 << 12 )
#define HCD_CHANNEL_INTERRUPT_FRAME_LIST_ROLLOVER ( 1 << 13 )
#define HCD_CHANNEL_INTERRUPT_RESERVED 0xffffc000

#endif
