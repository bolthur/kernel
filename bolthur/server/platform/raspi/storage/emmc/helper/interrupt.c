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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "../emmc.h"
// from iomem
#include "../../../libemmc.h"
#include "../../../libiomem.h"
#include "../../../libperipheral.h"

/**
 * @fn emmc_response_t emmc_mask_interrupt(uint32_t)
 * @brief Helper to mask interrupts
 *
 * @param mask
 * @return
 */
emmc_response_t emmc_mask_interrupt( uint32_t mask ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // overwrite interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ 0 ].value = mask;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_mask_interrupt(uint32_t)
 * @brief Helper to mask interrupts
 *
 * @param mask
 * @return
 */
emmc_response_t emmc_mask_interrupt_mask( uint32_t mask ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 2, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // overwrite interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 1 ].value = mask;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_reset_interrupt(uint32_t)
 * @brief Reset interrupts
 *
 * @param mask
 * @return
 */
emmc_response_t emmc_reset_interrupt( uint32_t mask ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 3, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // Send all interrupts to arm
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_IRPT_ENABLE;
  sequence[ 0 ].value = mask;
  // reset interrupt flags
  sequence[ 1 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 1 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  sequence[ 1 ].value = 0xFFFFFFFF;
  // apply mask all
  sequence[ 2 ].type = IOMEM_MMIO_ACTION_WRITE;
  sequence[ 2 ].offset = PERIPHERAL_EMMC_IRPT_MASK;
  sequence[ 2 ].value = 0xFFFFFFFF;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn emmc_response_t emmc_interrupt_fetch(uint32_t*)
 * @brief Helper to fetch interrupts from register
 *
 * @param destination
 * @return
 */
emmc_response_t emmc_interrupt_fetch( uint32_t* destination ) {
  // allocate sequence
  size_t sequence_size;
  iomem_mmio_entry_ptr_t sequence = emmc_prepare_sequence( 1, &sequence_size );
  if ( ! sequence ) {
    return EMMC_RESPONSE_ERROR_MEMORY;
  }
  // read interrupt register
  sequence[ 0 ].type = IOMEM_MMIO_ACTION_READ;
  sequence[ 0 ].offset = PERIPHERAL_EMMC_INTERRUPT;
  // perform request
  int result = ioctl(
    device->fd_iomem,
    IOCTL_BUILD_REQUEST(
      IOMEM_MMIO,
      sequence_size,
      IOCTL_RDWR
    ),
    sequence
  );
  // handle ioctl error
  if ( -1 == result ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "mmio rpc failed\r\n" )
    #endif
    free( sequence );
    return EMMC_RESPONSE_ERROR_IO;
  }
  // return value
  if ( destination ) {
    *destination = sequence[ 0 ].value;
  }
  free( sequence );
  // return success
  return EMMC_RESPONSE_OK;
}

/**
 * @fn void emmc_interrupt_handle_card(void)
 * @brief Handle card interrupts
 */
void emmc_interrupt_handle_card( void ) {
  emmc_response_t response;
  // get card status
  if ( device->card_rca ) {
    response = emmc_issue_command_ex(
      emmc_command_list[ EMMC_CMD_SEND_STATUS ],
      device->card_rca << 16,
      500000
    );
    if ( EMMC_RESPONSE_OK != response ) {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "Card status fetch failed!\r\n" )
      #endif
    } else {
      // debug output
      #if defined( EMMC_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT(
          "Card status: %#"PRIx32"\r\n",
          device->last_response[ 0 ]
       )
      #endif
    }
  }
}

/**
 * @fn void emmc_interrupt_handle(void)
 * @brief Handle controller interrupts
 */
void emmc_interrupt_handle( void ) {
  // variable stuff
  uint32_t interrupt;
  emmc_response_t response;
  uint32_t reset = 0;
  // get interrupt register
  do {
    response = emmc_interrupt_fetch( &interrupt );
  } while ( EMMC_RESPONSE_OK != response );

  // command complete
  if ( interrupt & EMMC_INTERRUPT_CMD_DONE ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command completed!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_CMD_DONE;
  }

  // transfer done
  if ( interrupt & EMMC_INTERRUPT_DATA_DONE ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Data transfer completed!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_DATA_DONE;
  }

  // block gap
  if ( interrupt & EMMC_INTERRUPT_BLOCK_GAP ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Block gap interrupt!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_BLOCK_GAP;
  }

  // write ready
  if ( interrupt & EMMC_INTERRUPT_WRITE_RDY ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Write ready interrupt!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_WRITE_RDY;
    // reset data
    while( EMMC_RESPONSE_OK != emmc_reset_data() ) {
      __asm__ __volatile__( "nop" );
    }
  }

  // read ready
  if ( interrupt & EMMC_INTERRUPT_READ_RDY ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Read ready interrupt!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_READ_RDY;
  }

  // card interrupt
  if ( interrupt & EMMC_INTERRUPT_CARD ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Card interrupt!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_CARD;
    // handle card interrupts
    emmc_interrupt_handle_card();
  }

  // retune
  if ( interrupt & EMMC_INTERRUPT_RETUNE ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Retune interrupt!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_RETUNE;
  }

  // boot ack
  if ( interrupt & EMMC_INTERRUPT_BOOTACK ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Boot acknowledge!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_BOOTACK;
  }

  // endboot
  if ( interrupt & EMMC_INTERRUPT_ENDBOOT ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Boot operation terminated!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_ENDBOOT;
  }

  // err
  if ( interrupt & EMMC_INTERRUPT_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Some generic error!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_ERR;
  }

  // cto
  if ( interrupt & EMMC_INTERRUPT_CTO_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command timeout!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_CTO_ERR;
  }

  // ccrc
  if ( interrupt & EMMC_INTERRUPT_CCRC_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Command CRC error!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_CCRC_ERR;
  }

  // cend
  if ( interrupt & EMMC_INTERRUPT_CEND_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "End bit of command not 1!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_CEND_ERR;
  }

  // cbad
  if ( interrupt & EMMC_INTERRUPT_CBAD_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Incorrect command index in response!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_CBAD_ERR;
  }

  // dto
  if ( interrupt & EMMC_INTERRUPT_DTO_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Data transfer timeout!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_DTO_ERR;
  }

  // dcrc
  if ( interrupt & EMMC_INTERRUPT_DCRC_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Data CRC error!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_DCRC_ERR;
  }

  // dend
  if ( interrupt & EMMC_INTERRUPT_DEND_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "End bit of data not 1!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_DEND_ERR;
  }

  // acmd
  if ( interrupt & EMMC_INTERRUPT_ACMD_ERR ) {
    // debug output
    #if defined( EMMC_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Auto command error!\r\n" )
    #endif
    // set reset mask
    reset |= EMMC_INTERRUPT_ACMD_ERR;
  }

  // write back reset
  while ( EMMC_RESPONSE_OK != emmc_mask_interrupt( reset ) ) {
    __asm__ __volatile__ ( "nop" );
  }
}
