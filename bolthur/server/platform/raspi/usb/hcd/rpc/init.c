/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

// system includes
#include <errno.h>
// local includes
#include "../rpc.h"
#include "../global.h"
// driver includes
#include "../../../libhcd.h"
#include "../../../../../libhcd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // default handler
  bolthur_rpc_bind( RPC_TIMER, rpc_default_timer, true );
  if ( errno ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register handler timer!\r\n" )
    #endif
    return false;
  }
  // bind interrupt handler
  bolthur_rpc_bind( ARM_IRQ_USB, rpc_interrupt_handle, true );
  if ( errno ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register handler read!\r\n" )
    #endif
    return false;
  }
  // bind rpc handler for communication
  bolthur_rpc_bind( HCD_SUBMIT_CONTROL_MESSAGE, rpc_submit_message, true );
  if ( errno ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register handler submit control message!\r\n" )
    #endif
    return false;
  }
  // bind rpc handler for communication
  bolthur_rpc_bind( HCD_POLL_INTERRUPT, rpc_poll_interrupt, true );
  if ( errno ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register handler poll interrupt!\r\n" )
    #endif
    return false;
  }
  // bind rpc handler for communication
  bolthur_rpc_bind( HCD_STOP_TRANSMISSION, rpc_stop_transmission, true );
  if ( errno ) {
    #if defined( HCD_ENABLE_OUTPUT )
      STARTUP_PRINT( "Unable to register handler stop transmission!\r\n" )
    #endif
    return false;
  }
  // return success
  return true;
}
