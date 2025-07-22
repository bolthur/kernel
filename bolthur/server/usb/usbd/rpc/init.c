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
#include "../../../libusb.h"

/**
 * @fn bool rpc_init(void)
 * @brief Init rpc handler method
 * @return
 */
bool rpc_init( void ) {
  // register handler register
  bolthur_rpc_bind( USBD_REGISTER_DEVICE_HANDLER, rpc_handler_register, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register handler register device handler!\r\n" )
    return false;
  }
  // register handler unregister
  bolthur_rpc_bind( USBD_UNREGISTER_DEVICE_HANDLER, rpc_handler_unregister, true );
  if ( errno ) {
    STARTUP_PRINT( "Unable to register handler unregister device handler!\r\n" )
    return false;
  }
  return true;
}
