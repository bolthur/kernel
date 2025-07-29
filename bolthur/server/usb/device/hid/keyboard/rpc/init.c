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

#include <errno.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../../../../../libusbd.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register handler keyboard attach
  bolthur_rpc_bind( GENERIC_ATTACH, rpc_keyboard_attach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register attach handler!\r\n" )
    return false;
  }
  // register handler detach
  bolthur_rpc_bind( GENERIC_DETACH, rpc_keyboard_detach, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register detach handler!\r\n" )
    return false;
  }
  // register handler deallocate
  bolthur_rpc_bind( GENERIC_DEALLOCATE, rpc_keyboard_deallocate, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register deallocate handler!\r\n" )
    return false;
  }
  return true;
}
