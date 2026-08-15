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

#include "../../rpc.h"
#include "../../dwhci.h"

/**
 * @fn void rpc_default_timer(size_t, pid_t, size_t, size_t)
 * @brief Default handler for timer
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_default_timer(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  size_t response_info
) {
  // clear timer if set
  if ( response_info ) {
    _syscall_timer_release( response_info );
  }
  // try to find entry with matching timer
  auto entry = configuration.list;
  while ( entry ) {
    // handle match
    if ( entry->timer == response_info ) {
      break;
    }
    // switch to next
    entry = entry->next;
  }
  // handle no match
  if ( ! entry ) {
    return;
  }
  // debug output
  #if defined( DWHCI_ENABLE_DEBUG )
    EARLY_STARTUP_PRINT( "Timeout reached\r\n" )
  #endif
  // switch status and cancel channel
  entry->status = DWHCI_QUEUE_CANCEL;
  // start cancellation
  const response_t response = dwhci_channel_async_continue( entry );
  if ( HCD_RESPONSE_OK != response ) {
    // debug output
    #if defined ( DWHCI_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to start cancellation process\r\n" )
    #endif
  }
}
