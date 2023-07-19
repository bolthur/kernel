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

#include <inttypes.h>
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/bolthur.h>
#include "dma.h"
#include "mmio.h"
#include "barrier.h"
#include "../libdma.h"
#include "../libperipheral.h"
#include "generic.h"

static dma_control_block_t* block = NULL;

static int dma_block_init( dma_control_block_t** to_save ) {
  // allocate control block
  block = mmap(
    NULL,
    sizeof( *block ),
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_DEVICE,
    -1,
    0
  );
  if ( MAP_FAILED == block ) {
    errno = ENOMEM;
    return -1;
  }
  // clear out
  memset( block, 0, sizeof( *block ) );
  // push to to save
  *to_save = block;
  // return block
  return 0;
}

/**
 * @fn int dma_block_prepare(void)
 * @brief Prepare dma block
 *
 * @return
 */
int dma_block_prepare( void ) {
  memset( block, 0, sizeof( *block ) );
  return 0;
}

/**
 * @fn int dma_block_to_phys(uintptr_t*)
 * @brief Translate control block to bus address
 *
 * @param block
 * @param addr
 * @return
 *
 * @todo validate parameter
 */
int dma_block_to_phys( uintptr_t* addr ) {
  // translate to physical
  uintptr_t bus = _syscall_memory_translate_physical( ( uintptr_t )block );
  // handle error
  if ( errno ) {
    EARLY_STARTUP_PRINT( "error: %s\r\n", strerror( errno ) );
    return -1;
  }
  // set address
  *addr = bus;
  // return bus address
  return 0;
}

/**
 * @fn int dma_block_set_address(uint32_t, uint32_t)
 * @brief Set dma block address
 *
 * @param source
 * @param destination
 * @return
 *
 * @todo move magic value 0xc0000000 to constant
 * @todo validate parameter
 */
int dma_block_set_address(
  uint32_t source,
  uint32_t destination
) {
  block->source_address = source;
  block->destination_address = destination;
  return 0;
}

/**
 * @fn int dma_block_transfer_info_source_increment(bool)
 * @brief Set source increment information
 *
 * @param value
 * @return
 */
int dma_block_transfer_info_source_increment( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_SRC_INC;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_SRC_INC );
  }
  return 0;
}

/**
 * @fn int dma_block_transfer_info_destination_increment(bool)
 * @brief Set destination increment information
 *
 * @param value
 * @return
 */
int dma_block_transfer_info_destination_increment( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_DEST_INC;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_DEST_INC );
  }
  return 0;
}

/**
 * @fn int dma_block_transfer_info_wait_response(dma_control_block_t*, bool)
 * @brief Set or remove wait response
 *
 * @param value
 * @return
 */
int dma_block_transfer_info_wait_response( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_WAIT_RESP;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_WAIT_RESP );
  }
  return 0;
}

int dma_block_transfer_info_src_width( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_SRC_WIDTH;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_SRC_WIDTH );
  }
  return 0;
}

int dma_block_transfer_info_dest_width( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_DEST_WIDTH;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_DEST_WIDTH );
  }
  return 0;
}

int dma_block_transfer_info_src_dreq( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_SRC_DREQ;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_SRC_DREQ );
  }
  return 0;
}

int dma_block_transfer_info_dest_dreq( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_DEST_DREQ;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_DEST_DREQ );
  }
  return 0;
}

int dma_block_transfer_info_interrupt_enable( bool value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_INTEN;
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_INTEN );
  }
  return 0;
}

int dma_block_transfer_info_permap( uint32_t value ) {
  if ( value ) {
    block->transfer_information |= LIBDMA_TI_PREAPRE_PERMAP( value );
  } else {
    block->transfer_information &= ( uint32_t )( ~LIBDMA_TI_PREAPRE_PERMAP( value ) );
  }
  return 0;
}

/**
 * @fn int dma_block_set_transfer_length(uint32_t)
 * @brief Set transfer length information
 *
 * @param block
 * @param value
 * @return
 */
int dma_block_set_transfer_length( uint32_t value ) {
  block->transfer_length = value;
  return 0;
}

/**
 * @fn int dma_block_set_stride(uint32_t)
 * @brief Set stride
 *
 * @param block
 * @param value
 * @return
 */
int dma_block_set_stride( uint32_t value ) {
  block->stride = value;
  return 0;
}

/**
 * @fn int dma_block_set_next(uint32_t)
 * @brief set next control block
 *
 * @param block
 * @param value
 * @return
 */
int dma_block_set_next( uint32_t value ) {
  block->next_control_block = value;
  return 0;
}

/**
 * @fn void dma_reset_channel(uint32_t)
 * @brief Reset channel helper
 *
 * @param channel_cs
 */
static void dma_reset_channel( uint32_t channel_cs ) {
  // set reset and wait for outstanding writes
  mmio_write(
    channel_cs,
    ( uint32_t )( LIBDMA_CS_RESET | LIBDMA_CS_WAIT_FOR_OUTSTANDING_WRITES )
  );
  // wait until reset bit clears
  // wait until reset bit clears
  while ( mmio_read( channel_cs ) & ( uint32_t )LIBDMA_CS_RESET ) {
    __asm__ __volatile__( "nop" );
  }
}

/**
 * @fn void dma_block_dump(void)
 * @brief Dump dma block
 *
 * @param block
 */
void dma_block_dump( void ) {
  EARLY_STARTUP_PRINT( "block->destination_address: %#"PRIx32"\r\n", block->destination_address );
  EARLY_STARTUP_PRINT( "block->next_control_block: %#"PRIx32"\r\n", block->next_control_block );
  EARLY_STARTUP_PRINT( "block->source_address: %#"PRIx32"\r\n", block->source_address );
  EARLY_STARTUP_PRINT( "block->stride: %#"PRIx32"\r\n", block->stride );
  EARLY_STARTUP_PRINT( "block->transfer_information: %#"PRIx32"\r\n", block->transfer_information );
  EARLY_STARTUP_PRINT( "block->transfer_length: %#"PRIx32"\r\n", block->transfer_length );
}

/**
 * @fn void dma_init(void)
 * @brief Setup dma handling
 *
 */
void dma_init( void ) {
  // reset all channels
  dma_reset_channel( PERIPHERAL_DMA0_CS );
  dma_reset_channel( PERIPHERAL_DMA1_CS );
  dma_reset_channel( PERIPHERAL_DMA2_CS );
  dma_reset_channel( PERIPHERAL_DMA3_CS );
  dma_reset_channel( PERIPHERAL_DMA4_CS );
  dma_reset_channel( PERIPHERAL_DMA5_CS );
  dma_reset_channel( PERIPHERAL_DMA6_CS );
  dma_reset_channel( PERIPHERAL_DMA7_CS );
  dma_reset_channel( PERIPHERAL_DMA8_CS );
  dma_reset_channel( PERIPHERAL_DMA9_CS );
  dma_reset_channel( PERIPHERAL_DMA10_CS );
  dma_reset_channel( PERIPHERAL_DMA11_CS );
  dma_reset_channel( PERIPHERAL_DMA12_CS );
  dma_reset_channel( PERIPHERAL_DMA13_CS );
  dma_reset_channel( PERIPHERAL_DMA14_CS );
  // disable all
  mmio_write( PERIPHERAL_DMA_ENABLE, 0 );
  // init dma block
  dma_block_init( &block );
}

/**
 * @fn int dma_start(void)
 * @brief Method to start dma copy
 *
 * @return
 */
int dma_start( void ) {
  // translate to bus address
  uintptr_t phys;
  if ( 0 != dma_block_to_phys( &phys ) ) {
    //EARLY_STARTUP_PRINT( "dma block to phys failed!\r\n" )
    return -1;
  }
  //dma_block_dump();
  //EARLY_STARTUP_PRINT( "block = %#"PRIxPTR"\r\n", ( uintptr_t )block )
  //EARLY_STARTUP_PRINT( "phys = %#"PRIxPTR"\r\n", phys )
  //EARLY_STARTUP_PRINT( "Activate dma channel 0\r\n" )
  // activate dma
  uint32_t value = mmio_read( PERIPHERAL_DMA_ENABLE );
  value |= 1 << 0;
  mmio_write( PERIPHERAL_DMA_ENABLE, value );
  //EARLY_STARTUP_PRINT( "Reset dma channel 0\r\n" )
  dma_reset_channel( PERIPHERAL_DMA0_CS );
  //dma_dump();
  //EARLY_STARTUP_PRINT( "Set conblk address\r\n" )
  // write callback
  mmio_write( PERIPHERAL_DMA0_CONBLK_AD, ( uint32_t )phys | 0xC0000000 );
  //dma_dump();
  //EARLY_STARTUP_PRINT( "Activate in CS\r\n" )
  // activate
  //value = mmio_read( PERIPHERAL_DMA0_CS );
  mmio_write(
    PERIPHERAL_DMA0_CS,
    LIBDMA_CS_DISDEBUG | LIBDMA_CS_ACTIVE | LIBDMA_CS_END | LIBDMA_CS_INT
  );
  //dma_dump();
  // return success
  return 0;
}

/**
 * @fn int dma_wait(void)
 * @brief Method to wait until dma finished
 *
 * @return
 */
int dma_wait( void ) {
  // dump
  //dma_dump();
  // debug output
  //EARLY_STARTUP_PRINT( "waiting for active copy ends\r\n" )
  uint32_t value;
  do {
    // fetch value
    value = mmio_read( PERIPHERAL_DMA0_CS );
    // check for error
    if ( value & LIBDMA_CS_ERROR ) {
      //EARLY_STARTUP_PRINT( "DMA ERROR!\r\n" )
      errno = EIO;
      return -1;
    }
  } while ( ! ( value & LIBDMA_CS_END ) );
  // dump
  //dma_dump();
  //EARLY_STARTUP_PRINT( "done\r\n" )
  return 0;
}

/**
 * @fn int dma_finish(void)
 * @brief Method too finish dma transfer
 *
 * @return
 */
int dma_finish( void ) {
  // read value
  uint32_t value = mmio_read( PERIPHERAL_DMA0_CS );
  // activate dma
  mmio_write( PERIPHERAL_DMA0_CS, value & ( uint32_t )( ~LIBDMA_CS_ACTIVE ) );
  // read value
  value = mmio_read( PERIPHERAL_DMA_ENABLE );
  // deactivate dma
  mmio_write( PERIPHERAL_DMA_ENABLE, value & ( uint32_t )( ~( 1 << 0 ) ) );
  // return success
  return 0;
}

/**
 * @fn void dma_dump(void)
 * @brief Dump dma registers
 */
void dma_dump( void ) {
  uint32_t value = mmio_read( PERIPHERAL_DMA0_CS );
  EARLY_STARTUP_PRINT( "CS: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_CONBLK_AD );
  EARLY_STARTUP_PRINT( "CONBLK_AD: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_TI );
  EARLY_STARTUP_PRINT( "TI: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_SOURCE_AD );
  EARLY_STARTUP_PRINT( "SOURCE_AD: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_DEST_AD );
  EARLY_STARTUP_PRINT( "DEST_AD: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_TXFR_LEN );
  EARLY_STARTUP_PRINT( "TXFR_LEN: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_STRIDE );
  EARLY_STARTUP_PRINT( "STRIDE: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_NEXTCONBK );
  EARLY_STARTUP_PRINT( "NEXTCONBK: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA0_DEBUG );
  EARLY_STARTUP_PRINT( "DEBUG: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA_INT_STATUS );
  EARLY_STARTUP_PRINT( "INT_STATUS: %#"PRIx32"\r\n", value );
  value = mmio_read( PERIPHERAL_DMA_ENABLE );
  EARLY_STARTUP_PRINT( "ENABLE: %#"PRIx32"\r\n", value );
}
