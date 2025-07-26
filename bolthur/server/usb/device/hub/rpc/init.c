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

// system includes
#include <errno.h>
// local includes
#include "../rpc.h"
#include "../../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register attach handler
  bolthur_rpc_bind( GENERIC_ATTACH, rpc_hub_attach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register attach handler!\r\n" )
    return false;
  }
  // register detach handler
  bolthur_rpc_bind( GENERIC_DETACH, rpc_hub_detach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register detach handler!\r\n" )
    return false;
  }
  // register deallocate handler
  bolthur_rpc_bind( GENERIC_DEALLOCATE, rpc_hub_deallocate, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register deallocate handler!\r\n" )
    return false;
  }
  // register check change handler
  bolthur_rpc_bind( HUB_CHECK_CHANGE, rpc_hub_check_change, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register check change!\r\n" )
    return false;
  }
  // register child detach handler
  bolthur_rpc_bind( HUB_CHILD_DETACH, rpc_hub_child_detach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register child detach!\r\n" )
    return false;
  }
  // register child reset handler
  bolthur_rpc_bind( HUB_CHILD_RESET, rpc_hub_child_reset, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register child reset!\r\n" )
    return false;
  }
  // register check connection handler
  bolthur_rpc_bind( HUB_CHECK_CONNECTION, rpc_hub_check_connection, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register check connection!\r\n" )
    return false;
  }
  // return success
  return true;
}
