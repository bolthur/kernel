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

#if ! defined( _LIBEMMC_H )
#define _LIBEMMC_H

// command register
#define EMMC_CMDTM_CMD_TYPE_NORMAL ( 0 << 22 )
#define EMMC_CMDTM_CMD_TYPE_SUSPEND ( 1 << 22 )
#define EMMC_CMDTM_CMD_TYPE_RESUME ( 2 << 22 )
#define EMMC_CMDTM_CMD_TYPE_ABORT ( 3 << 22 )
#define EMMC_CMDTM_CMD_ISDATA ( 1 << 21 )
#define EMMC_CMDTM_CMD_IXCHK_EN ( 1 << 20 )
#define EMMC_CMDTM_CMD_CRCCHK_EN ( 1 << 19 )
#define EMMC_CMDTM_CMD_RSPNS_TYPE_NONE ( 0 << 16 )
#define EMMC_CMDTM_CMD_RSPNS_TYPE_136 ( 1 << 16 )
#define EMMC_CMDTM_CMD_RSPNS_TYPE_48 ( 2 << 16 )
#define EMMC_CMDTM_CMD_RSPNS_TYPE_48B ( 3 << 16 )
#define EMMC_CMDTM_CMD_TM_MULTI_BLOCK ( 1 << 5 )
#define EMMC_CMDTM_CMD_TM_DAT_DIR_HC ( 0 << 4 )
#define EMMC_CMDTM_CMD_TM_DAT_DIR_CH ( 1 << 4 )
#define EMMC_CMDTM_CMD_TM_AUTO_CMD_EN_NONE ( 0 << 2 )
#define EMMC_CMDTM_CMD_TM_AUTO_CMD_EN_CMD12 ( 1 << 2 )
#define EMMC_CMDTM_CMD_TM_AUTO_CMD_EN_CMD23 ( 2 << 2 )
#define EMMC_CMDTM_CMD_TM_AUTO_CMD_EN_RESERVED ( 3 << 2 )
#define EMMC_CMDTM_CMD_TM_BLKCNT_EN ( 1 << 1 )
#define EMMC_CMDTM_CMD_TM_DMA_ENABLE ( 1 << 0 ) // Taken from hss not in raspi documentation
// status register
#define EMMC_STATUS_READ_TRANSFER ( 1 << 9 )
#define EMMC_STATUS_WRITE_TRANSFER ( 1 << 8 )
#define EMMC_STATUS_DAT_ACTIVE ( 1 << 2 )
#define EMMC_STATUS_DAT_INHIBIT ( 1 << 1 )
#define EMMC_STATUS_CMD_INHIBIT ( 1 << 0 )
// control0 register
#define EMMC_CONTROL0_ALT_BOOT_EN ( 1 << 22 )
#define EMMC_CONTROL0_BOOT_EN ( 1 << 21 )
#define EMMC_CONTROL0_SPI_MODE ( 1 << 20 )
#define EMMC_CONTROL0_GAP_IEN ( 1 << 19 )
#define EMMC_CONTROL0_READWAIT_EN ( 1 << 18 )
#define EMMC_CONTROL0_GAP_WAIT ( 1 << 17 )
#define EMMC_CONTROL0_GAP_STOP ( 1 << 16 )
#define EMMC_CONTROL0_HCTL_8BIT ( 1 << 5 )
#define EMMC_CONTROL0_HCTL_HS_EN ( 1 << 2 )
#define EMMC_CONTROL0_HCTL_DWIDTH ( 1 << 1 )
// control1 register
#define EMMC_CONTROL1_SRST_DATA ( 1 << 26 )
#define EMMC_CONTROL1_SRST_CMD ( 1 << 25 )
#define EMMC_CONTROL1_SRST_HC ( 1 << 24 )
#define EMMC_CONTROL1_CLK_EN ( 1 << 2 )
#define EMMC_CONTROL1_CLK_STABLE ( 1 << 1 )
#define EMMC_CONTROL1_CLK_INTLEN ( 1 << 0 )
// interrupt register
#define EMMC_INTERRUPT_ACMD_ERR ( 1 << 24 )
#define EMMC_INTERRUPT_DEND_ERR ( 1 << 22 )
#define EMMC_INTERRUPT_DCRC_ERR ( 1 << 21 )
#define EMMC_INTERRUPT_DTO_ERR ( 1 << 20 )
#define EMMC_INTERRUPT_CBAD_ERR ( 1 << 19 )
#define EMMC_INTERRUPT_CEND_ERR ( 1 << 18 )
#define EMMC_INTERRUPT_CCRC_ERR ( 1 << 17 )
#define EMMC_INTERRUPT_CTO_ERR ( 1 << 16 )
#define EMMC_INTERRUPT_ERR ( 1 << 15 )
#define EMMC_INTERRUPT_ENDBOOT ( 1 << 14 )
#define EMMC_INTERRUPT_BOOTACK ( 1 << 13 )
#define EMMC_INTERRUPT_RETUNE ( 1 << 12 )
#define EMMC_INTERRUPT_CARD ( 1 << 8 )
#define EMMC_INTERRUPT_READ_RDY ( 1 << 5 )
#define EMMC_INTERRUPT_WRITE_RDY ( 1 << 4 )
#define EMMC_INTERRUPT_BLOCK_GAP ( 1 << 2 )
#define EMMC_INTERRUPT_DATA_DONE ( 1 << 1 )
#define EMMC_INTERRUPT_CMD_DONE ( 1 << 0 )
// interrupt mask register
#define EMMC_IRPT_MASK_ACMD_ERR ( 1 << 24 )
#define EMMC_IRPT_MASK_DEND_ERR ( 1 << 22 )
#define EMMC_IRPT_MASK_DCRC_ERR ( 1 << 21 )
#define EMMC_IRPT_MASK_DTO_ERR ( 1 << 20 )
#define EMMC_IRPT_MASK_CBAD_ERR ( 1 << 19 )
#define EMMC_IRPT_MASK_CEND_ERR ( 1 << 18 )
#define EMMC_IRPT_MASK_CCRC_ERR ( 1 << 17 )
#define EMMC_IRPT_MASK_CTO_ERR ( 1 << 16 )
#define EMMC_IRPT_MASK_ERR ( 1 << 15 )
#define EMMC_IRPT_MASK_ENDBOOT ( 1 << 14 )
#define EMMC_IRPT_MASK_BOOTACK ( 1 << 13 )
#define EMMC_IRPT_MASK_RETUNE ( 1 << 12 )
#define EMMC_IRPT_MASK_CARD ( 1 << 8 )
#define EMMC_IRPT_MASK_READ_RDY ( 1 << 5 )
#define EMMC_IRPT_MASK_WRITE_RDY ( 1 << 4 )
#define EMMC_IRPT_MASK_BLOCK_GAP ( 1 << 2 )
#define EMMC_IRPT_MASK_DATA_DONE ( 1 << 1 )
#define EMMC_IRPT_MASK_CMD_DONE ( 1 << 0 )
// interrupt enable register
#define EMMC_IRPT_ENABLE_ACMD_ERR ( 1 << 24 )
#define EMMC_IRPT_ENABLE_DEND_ERR ( 1 << 22 )
#define EMMC_IRPT_ENABLE_DCRC_ERR ( 1 << 21 )
#define EMMC_IRPT_ENABLE_DTO_ERR ( 1 << 20 )
#define EMMC_IRPT_ENABLE_CBAD_ERR ( 1 << 19 )
#define EMMC_IRPT_ENABLE_CEND_ERR ( 1 << 18 )
#define EMMC_IRPT_ENABLE_CCRC_ERR ( 1 << 17 )
#define EMMC_IRPT_ENABLE_CTO_ERR ( 1 << 16 )
#define EMMC_IRPT_ENABLE_ENDBOOT ( 1 << 14 )
#define EMMC_IRPT_ENABLE_BOOTACK ( 1 << 13 )
#define EMMC_IRPT_ENABLE_RETUNE ( 1 << 12 )
#define EMMC_IRPT_ENABLE_CARD ( 1 << 8 )
#define EMMC_IRPT_ENABLE_READ_RDY ( 1 << 5 )
#define EMMC_IRPT_ENABLE_WRITE_RDY ( 1 << 4 )
#define EMMC_IRPT_ENABLE_BLOCK_GAP ( 1 << 2 )
#define EMMC_IRPT_ENABLE_DATA_DONE ( 1 << 1 )
#define EMMC_IRPT_ENABLE_CMD_DONE ( 1 << 0 )
// control2 register
#define EMMC_CONTROL2_TUNED ( 1 << 23 )
#define EMMC_CONTROL2_TUNEON ( 1 << 22 )
#define EMMC_CONTROL2_UHSMODE_SDR12 ( 0 << 16 )
#define EMMC_CONTROL2_UHSMODE_SDR25 ( 1 << 16 )
#define EMMC_CONTROL2_UHSMODE_SDR50 ( 2 << 16 )
#define EMMC_CONTROL2_UHSMODE_SDR104 ( 3 << 16 )
#define EMMC_CONTROL2_UHSMODE_DDR50 ( 4 << 16 )
#define EMMC_CONTROL2_NOTC12_ERR ( 1 << 7 )
#define EMMC_CONTROL2_ACBAD_ERR ( 1 << 4 )
#define EMMC_CONTROL2_ACEND_ERR ( 1 << 3 )
#define EMMC_CONTROL2_ACCRC_ERR ( 1 << 2 )
#define EMMC_CONTROL2_ACTO_ERR ( 1 << 1 )
#define EMMC_CONTROL2_ACNOX_ERR ( 1 << 0 )
// force interrupt register
#define EMMC_FORCE_IRPT_ACMD_ERR ( 1 << 24 )
#define EMMC_FORCE_IRPT_DEND_ERR ( 1 << 22 )
#define EMMC_FORCE_IRPT_DCRC_ERR ( 1 << 21 )
#define EMMC_FORCE_IRPT_DTO_ERR ( 1 << 20 )
#define EMMC_FORCE_IRPT_CBAD_ERR ( 1 << 19 )
#define EMMC_FORCE_IRPT_CEND_ERR ( 1 << 18 )
#define EMMC_FORCE_IRPT_CCRC_ERR ( 1 << 17 )
#define EMMC_FORCE_IRPT_CTO_ERR ( 1 << 16 )
#define EMMC_FORCE_IRPT_ENDBOOT ( 1 << 14 )
#define EMMC_FORCE_IRPT_BOOTACK ( 1 << 13 )
#define EMMC_FORCE_IRPT_RETUNE ( 1 << 12 )
#define EMMC_FORCE_IRPT_CARD ( 1 << 8 )
#define EMMC_FORCE_IRPT_READ_RDY ( 1 << 5 )
#define EMMC_FORCE_IRPT_WRITE_RDY ( 1 << 4 )
#define EMMC_FORCE_IRPT_BLOCK_GAP ( 1 << 2 )
#define EMMC_FORCE_IRPT_DATA_DONE ( 1 << 1 )
#define EMMC_FORCE_IRPT_CMD_DONE ( 1 << 0 )
// SLOTISR_VER helper
#define SLOTISR_VER_VENDOR( a ) ( uint8_t )( ( ( a ) >> 24 ) & 0xff )
#define SLOTISR_VER_SDVERSION( a ) ( uint8_t )( ( ( a ) >> 16 ) & 0xff )
#define SLOTISR_VER_SLOT_STATUS( a ) ( uint8_t )( ( a ) & 0xff )

// helpers for command stuff
#define EMMC_CMD_INDEX( a ) ( (a) << 24 )
#define EMMC_CMD_RESERVED( a ) 0xFFFFFFFF
#define EMMC_CMD_WHO_KNOWS( a ) EMMC_CMD_RESERVED( a )
#define EMMC_CMD_UNUSED( a ) EMMC_CMD_RESERVED( a )
#define EMMC_APP_CMD_BIT ( 1u << 31 )
#define EMMC_APP_CMD_TO_CMD( a ) ( a & 0xFF )
#define EMMC_CMD_TO_APP_CMD( a ) ( EMMC_APP_CMD_BIT | a )
#define EMMC_CMD_IS_RESERVED( a ) ( a == EMMC_CMD_RESERVED( a ) )
#define EMMC_APP_CMD_INDEX( a ) ( EMMC_APP_CMD_BIT | EMMC_CMD_INDEX( a ) )
#define EMMC_IS_APP_CMD( a ) ( a & EMMC_APP_CMD_BIT )
#define EMMC_CMDTM_CMD_TYPE_MASK ( 3 << 22 )
#define EMMC_CMDTM_CMD_RSPNS_TYPE_MASK ( 3 << 16 )

// command response types
#define EMMC_CMD_RESPONSE_NONE EMMC_CMDTM_CMD_RSPNS_TYPE_NONE
#define EMMC_CMD_RESPONSE_R1 ( EMMC_CMDTM_CMD_RSPNS_TYPE_48 | EMMC_CMDTM_CMD_CRCCHK_EN )
#define EMMC_CMD_RESPONSE_R1b ( EMMC_CMDTM_CMD_RSPNS_TYPE_48B | EMMC_CMDTM_CMD_CRCCHK_EN )
#define EMMC_CMD_RESPONSE_R2 ( EMMC_CMDTM_CMD_RSPNS_TYPE_136 | EMMC_CMDTM_CMD_CRCCHK_EN )
#define EMMC_CMD_RESPONSE_R3 EMMC_CMDTM_CMD_RSPNS_TYPE_48
#define EMMC_CMD_RESPONSE_R4 EMMC_CMDTM_CMD_RSPNS_TYPE_136 /// FIXME: SHOULD BE 48
#define EMMC_CMD_RESPONSE_R5 ( EMMC_CMDTM_CMD_RSPNS_TYPE_48 | EMMC_CMDTM_CMD_CRCCHK_EN )
#define EMMC_CMD_RESPONSE_R6 ( EMMC_CMDTM_CMD_RSPNS_TYPE_48 | EMMC_CMDTM_CMD_CRCCHK_EN )
#define EMMC_CMD_RESPONSE_R7 ( EMMC_CMDTM_CMD_RSPNS_TYPE_48 | EMMC_CMDTM_CMD_CRCCHK_EN )

// command data read / write masks
#define EMMC_CMD_DATA_READ ( EMMC_CMDTM_CMD_ISDATA | EMMC_CMDTM_CMD_TM_DAT_DIR_CH )
#define EMMC_CMD_DATA_WRITE ( EMMC_CMDTM_CMD_ISDATA | EMMC_CMDTM_CMD_TM_DAT_DIR_HC )

// interrupt mask
#define EMMC_INTERRUPT_MASK 0xFFFF0000

// supported command types
#define EMMC_CMD_GO_IDLE_STATE 0
#define EMMC_CMD_ALL_SEND_CID 2
#define EMMC_CMD_SEND_RELATIVE_ADDR 3
#define EMMC_CMD_SET_DSR 4
#define EMMC_CMD_IO_SEND_OP_COND 5
#define EMMC_CMD_SWITCH_FUNC 6
#define EMMC_CMD_SELECT_CARD 7
#define EMMC_CMD_DESELECT_CARD EMMC_CMD_SELECT_CARD
#define EMMC_CMD_SEND_IF_COND 8
#define EMMC_CMD_SEND_CSD 9
#define EMMC_CMD_SEND_CID 10
#define EMMC_CMD_VOLTAGE_SWITCH 11
#define EMMC_CMD_STOP_TRANSMISSION 12
#define EMMC_CMD_SEND_STATUS 13
#define EMMX_CMD_SEND_TASK_STATUS EMMC_CMD_SEND_STATUS
#define EMMC_CMD_GO_INACTIVE_STATE 15
#define EMMC_CMD_SET_BLOCKLEN 16
#define EMMC_CMD_READ_SINGLE_BLOCK 17
#define EMMC_CMD_READ_MULTIPLE_BLOCK 18
#define EMMC_CMD_SEND_TUNING_BLOCK 19
#define EMMC_CMD_SPEED_CLASS_CONTROL 20
#define EMMC_CMD_ADDRESS_EXTENSION 22
#define EMMC_CMD_SET_BLOCK_COUNT 23
#define EMMC_CMD_WRITE_SINGLE_BLOCK 24
#define EMMC_CMD_WRITE_MULTIPLE_BLOCK 25
#define EMMC_CMD_PROGRAM_CSD 27
#define EMMC_CMD_SET_WRITE_PROT 28
#define EMMC_CMD_CLR_WRITE_PROT 29
#define EMMC_CMD_SEND_WRITE_PROT 30
#define EMMC_CMD_ERASE_WR_BLK_START 32
#define EMMC_CMD_ERASE_WR_BLK_END 33
#define EMMC_CMD_ERASE 38
#define EMMC_CMD_LOCK_UNLOCK 42
#define EMMC_CMD_APP_CMD 55
#define EMMC_CMD_GEN_CMD 56
// app command types
#define EMMC_APP_CMD_SET_BUS_WIDTH EMMC_CMD_TO_APP_CMD( 6 )
#define EMMC_APP_CMD_SD_STATUS EMMC_CMD_TO_APP_CMD( 13 )
#define EMMC_APP_CMD_SEND_NUM_WR_BLOCKS EMMC_CMD_TO_APP_CMD( 22 )
#define EMMC_APP_CMD_SET_WR_BLK_ERASE_COUNT EMMC_CMD_TO_APP_CMD( 23 )
#define EMMC_APP_CMD_SD_SEND_OP_COND EMMC_CMD_TO_APP_CMD( 41 )
#define EMMC_APP_CMD_SET_CLR_CARD_DETECT EMMC_CMD_TO_APP_CMD( 42 )
#define EMMC_APP_CMD_SEND_SCR EMMC_CMD_TO_APP_CMD( 51 )

// host controller versions
#define EMMC_HOST_CONTROLLER_V1 0
#define EMMC_HOST_CONTROLLER_V2 1
#define EMMC_HOST_CONTROLLER_V3 2
#define EMMC_HOST_CONTROLLER_V4 3
#define EMMC_HOST_CONTROLLER_V4_1 4
#define EMMC_HOST_CONTROLLER_V5 5

// sd card versions
#define EMMC_CARD_VERSION_UNKNOWN 0
#define EMMC_CARD_VERSION_1 1
#define EMMC_CARD_VERSION_1_1 2
#define EMMC_CARD_VERSION_2 3
#define EMMC_CARD_VERSION_3 4
#define EMMC_CARD_VERSION_4 5
#define EMMC_CARD_VERSION_5 6
#define EMMC_CARD_VERSION_6 7
#define EMMC_CARD_VERSION_7 8
#define EMMC_CARD_VERSION_8 9

// possible clock frequencies
#define EMMC_CLOCK_FREQUENCY_LOW 400000
#define EMMC_CLOCK_FREQUENCY_NORMAL 25000000
#define EMMC_CLOCK_FREQUENCY_HIGH 50000000
#define EMMC_CLOCK_FREQUENCY_100 100000000
#define EMMC_CLOCK_FREQUENCY_208 208000000

#endif
