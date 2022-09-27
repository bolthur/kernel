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

#include <libgen.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include "rpc.h"
#include "../libiomem.h"

struct mailbox_rpc command_list[] = {
  {
    .command = IOMEM_RPC_MAILBOX,
    .callback = rpc_handle_mailbox,
  },
  {
    .command = IOMEM_RPC_MMIO_PERFORM,
    .callback = rpc_handle_mmio_perform,
  },
  {
    .command = IOMEM_RPC_MMIO_LOCK,
    .callback = rpc_handle_mmio_lock,
  },
  {
    .command = IOMEM_RPC_MMIO_UNLOCK,
    .callback = rpc_handle_mmio_unlock,
  },
  {
    .command = IOMEM_RPC_GPIO_SET_FUNCTION,
    .callback = rpc_handle_gpio_set_function,
  },
  {
    .command = IOMEM_RPC_GPIO_SET_PULL,
    .callback = rpc_handle_gpio_set_pull,
  },
  {
    .command = IOMEM_RPC_GPIO_SET_DETECT,
    .callback = rpc_handle_gpio_set_detect,
  },
  {
    .command = IOMEM_RPC_GPIO_STATUS,
    .callback = rpc_handle_gpio_status,
  },
  {
    .command = IOMEM_RPC_GPIO_EVENT,
    .callback = rpc_handle_gpio_event,
  },
  {
    .command = IOMEM_RPC_GPIO_LOCK,
    .callback = rpc_handle_gpio_lock,
  },
  {
    .command = IOMEM_RPC_GPIO_UNLOCK,
    .callback = rpc_handle_gpio_unlock,
  },
};

/**
 * @fn bool rpc_register(void)
 * @brief Register necessary rpc handler
 *
 * @return
 */
bool rpc_register( void ) {
  // register all handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // register rpc
    bolthur_rpc_bind( command_list[ i ].command, command_list[ i ].callback, true );
    if ( errno ) {
      return false;
    }
  }
  return true;
}
