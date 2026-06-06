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

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
// local includes
#include "handler.h"
#include "../usbd.h"

/**
 * @brief Array of class handlers
 */
pid_t* class_handler;

/**
 * @fn int usbd_handler_init(void)
 * @brief Init handler
 * @return
 */
int usbd_handler_init( void ) {
  // allocate handler
  class_handler = calloc( 256, sizeof( pid_t ) );
  // handle error
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate memory\r\n" )
    #endif
    // return nomem
    return ENOMEM;
  }
  // clear out
  for ( size_t i = 0; i < 256; i++ ) {
    class_handler[i] = -1;
  }
  // return success
  return 0;
}

/**
 * @fn int usbd_handler_register(libusb_interface_class_t, pid_t)
 * @brief Method to register a handöer
 * @param type
 * @param handler
 * @return
 */
int usbd_handler_register( const libusb_interface_class_t type, const pid_t handler ) {
  // handle not initialized
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handler data not initialized\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // handle already set
  if ( -1 != class_handler[ type ] ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handler already registered\r\n" )
    #endif
    // return exist
    return EEXIST;
  }
  // set handler
  class_handler[ type ] = handler;
  // return success
  return 0;
}

/**
 * @fn int usbd_handler_unregister(libusb_interface_class_t, pid_t)
 * @brief Unregister a handler
 * @param type
 * @param handler
 * @return
 */
int usbd_handler_unregister( const libusb_interface_class_t type, const pid_t handler ) {
  // handle not initialized
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handler data not initialized\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // handle already set
  if ( handler != class_handler[ type ] ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handler already registered\r\n" )
    #endif
    // return exist
    return EINVAL;
  }
  // clear handler
  class_handler[ type ] = -1;
  // return success
  return 0;
}

/**
 * @fn int usbd_handler_get(libusb_interface_class_t, pid_t*)
 * @brief Method to get a bound handler
 * @param type
 * @param handler
 * @return
 */
int usbd_handler_get( const libusb_interface_class_t type, pid_t* handler ) {
  // handle not initialized
  if ( ! class_handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Handler data not initialized\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // handle no handler
  if ( ! handler ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Invalid handler passed\r\n" )
    #endif
    // return protocol error
    return EPROTO;
  }
  // set handler
  *handler = class_handler[ type ];
  // return success
  return 0;
}
