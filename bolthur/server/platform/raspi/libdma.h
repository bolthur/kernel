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

#if ! defined( _LIBDMA_H )
#define _LIBDMA_H

// control and status
#define LIBDMA_CS_ACTIVE ( 1 << 0 )
#define LIBDMA_CS_END ( 1 << 1 )
#define LIBDMA_CS_INT ( 1 << 2 )
#define LIBDMA_CS_DREQ ( 1 << 3 )
#define LIBDMA_CS_PAUSED ( 1 << 4 )
#define LIBDMA_CS_DREQ_STOPS_DMA ( 1 << 5 )
#define LIBDMA_CS_WAITING_FOR_OUTSTANDING_WRITES ( 1 << 6 )
#define LIBDMA_CS_ERROR ( 1 << 8 )
#define LIBDMA_CS_WAIT_FOR_OUTSTANDING_WRITES ( 1 << 28 )
#define LIBDMA_CS_DISDEBUG ( 1 << 29 )
#define LIBDMA_CS_ABORT ( 1 << 30 )
#define LIBDMA_CS_RESET ( 1 << 31 )

#define LIBDMA_CS_PREAPRE_PANIC_PRIORITY ( a ) ( ( a & 0xF ) << 20 )
#define LIBDMA_CS_EXTRACT_PANIC_PRIORITY ( a ) ( ( a >> 20 ) & 0xF )
#define LIBDMA_CS_PREAPRE_PRIORITY ( a ) ( ( a & 0xF ) << 16 )
#define LIBDMA_CS_EXTRACT_PRIORITY ( a ) ( ( a >> 16 ) & 0xF )
// transfer information
#define LIBDMA_TI_INTEN ( 1 << 0 )
#define LIBDMA_TI_TDMODE ( 1 << 1 )
#define LIBDMA_TI_WAIT_RESP ( 1 << 3 )
#define LIBDMA_TI_DEST_INC ( 1 << 4 )
#define LIBDMA_TI_DEST_WIDTH ( 1 << 5 )
#define LIBDMA_TI_DEST_DREQ ( 1 << 6 )
#define LIBDMA_TI_DEST_IGNORE ( 1 << 7 )
#define LIBDMA_TI_SRC_INC ( 1 << 8 )
#define LIBDMA_TI_SRC_WIDTH ( 1 << 9 )
#define LIBDMA_TI_SRC_DREQ ( 1 << 10 )
#define LIBDMA_TI_SRC_IGNORE ( 1 << 11 )

#define LIBDMA_TI_0_6_NO_WIDE_BURSTS ( 1 << 26 )

#define LIBDMA_TI_PERMAP_EMMC 11
#define LIBDMA_TI_PERMAP_SDHOST 13

#define LIBDMA_TI_PREAPRE_WAITS(a) ( ( ( a ) & 0x1F ) << 21 )
#define LIBDMA_TI_EXTRACT_WAITS(a) ( ( ( a ) >> 21 ) & 0x1F )
#define LIBDMA_TI_PREAPRE_PERMAP(a) ( ( ( a ) & 0x1F ) << 16 )
#define LIBDMA_TI_EXTRACT_PERMAP(a) ( ( ( a ) >> 16 ) & 0x1F )
#define LIBDMA_TI_PREAPRE_BURST_LENGTH(a) ( ( ( a ) & 0xF ) << 12 )
#define LIBDMA_TI_EXTRACT_BURST_LENGTH(a) ( ( ( a ) >> 12 ) & 0xF )
// debug register
#define LIBDMA_DEBUG_READ_LAST_NOT_SET_ERROR ( 1 << 0 )
#define LIBDMA_DEBUG_FIFO_ERROR ( 1 << 1 )
#define LIBDMA_DEBUG_READ_ERROR ( 1 << 2 )
#define LIBDMA_DEBUG_LITE ( 1 << 28 )
// interrupt status
#define LIBDMA_INT_STATUS0 ( 1 << 0 )
#define LIBDMA_INT_STATUS1 ( 1 << 1 )
#define LIBDMA_INT_STATUS2 ( 1 << 2 )
#define LIBDMA_INT_STATUS3 ( 1 << 3 )
#define LIBDMA_INT_STATUS4 ( 1 << 4 )
#define LIBDMA_INT_STATUS5 ( 1 << 5 )
#define LIBDMA_INT_STATUS6 ( 1 << 6 )
#define LIBDMA_INT_STATUS7 ( 1 << 7 )
#define LIBDMA_INT_STATUS8 ( 1 << 8 )
#define LIBDMA_INT_STATUS9 ( 1 << 9 )
#define LIBDMA_INT_STATUS10 ( 1 << 10 )
#define LIBDMA_INT_STATUS11 ( 1 << 11 )
#define LIBDMA_INT_STATUS12 ( 1 << 12 )
#define LIBDMA_INT_STATUS13 ( 1 << 13 )
#define LIBDMA_INT_STATUS14 ( 1 << 14 )
// interrupt enable
#define LIBDMA_ENABLE0 ( 1 << 0 )
#define LIBDMA_ENABLE1 ( 1 << 1 )
#define LIBDMA_ENABLE2 ( 1 << 2 )
#define LIBDMA_ENABLE3 ( 1 << 3 )
#define LIBDMA_ENABLE4 ( 1 << 4 )
#define LIBDMA_ENABLE5 ( 1 << 5 )
#define LIBDMA_ENABLE6 ( 1 << 6 )
#define LIBDMA_ENABLE7 ( 1 << 7 )
#define LIBDMA_ENABLE8 ( 1 << 8 )
#define LIBDMA_ENABLE9 ( 1 << 9 )
#define LIBDMA_ENABLE10 ( 1 << 10 )
#define LIBDMA_ENABLE11 ( 1 << 11 )
#define LIBDMA_ENABLE12 ( 1 << 12 )
#define LIBDMA_ENABLE13 ( 1 << 13 )
#define LIBDMA_ENABLE14 ( 1 << 14 )

#endif
