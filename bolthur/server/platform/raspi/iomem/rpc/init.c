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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "../rpc.h"
#include "../../libiomem.h"

/**
 * @fn bool rpc_init(void)
 * @brief Register necessary rpc handler
 *
 * @return
 */
bool rpc_init( void ) {
  bolthur_rpc_bind( IOMEM_RPC_MAILBOX, rpc_handle_mailbox, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_MMIO_PERFORM, rpc_handle_mmio_perform, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_MMIO_LOCK, rpc_handle_mmio_lock, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_MMIO_UNLOCK, rpc_handle_mmio_unlock, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_SET_FUNCTION, rpc_handle_gpio_set_function, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_SET_PULL, rpc_handle_gpio_set_pull, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_SET_DETECT, rpc_handle_gpio_set_detect, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_STATUS, rpc_handle_gpio_status, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_EVENT, rpc_handle_gpio_event, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_LOCK, rpc_handle_gpio_lock, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  bolthur_rpc_bind( IOMEM_RPC_GPIO_UNLOCK, rpc_handle_gpio_unlock, true );
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to register handler add!\r\n" )
    return false;
  }
  return true;
}
