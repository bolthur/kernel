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

#include <sys/bolthur.h>
#include <errno.h>
#include "lstat.h"

/**
 * @fn int lstat_handler(const char*, struct stat*, pid_t*)
 * @brief Helper to get handler from lstat response
 * @param pathname
 * @param buf
 * @param handler
 * @return
 */
int lstat_handler( const char* pathname, struct stat* buf, pid_t* handler ) {
  // variables
  vfs_stat_request_t* request = malloc( sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    errno = ENOMEM;
    return -1;
  }
  // clear message structures
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  // copy stuff to message
  strncpy( request->file_path, pathname, PATH_MAX - 1 );
  // raise rpc and wait for return
  const size_t response_id = bolthur_rpc_raise(
    RPC_VFS_STAT,
    VFS_DAEMON_ID,
    request,
    sizeof( vfs_stat_request_t ),
    NULL,
    RPC_VFS_STAT,
    request,
    sizeof( vfs_stat_request_t ),
    0,
    0,
    NULL,
    false
  );
  // handle error
  if ( 0 == response_id ) {
    free( request );
    return -1;
  }
  // get response
  size_t data_size;
  vfs_stat_response_t* response = bolthur_rpc_fetch_from_mailbox( response_id, &data_size, true, NULL );
  // handle error
  if ( ! response ) {
    free( request );
    return -1;
  }
  // handle failure
  if ( ! response->success ) {
    free( request );
    free( response );
    errno = EIO;
    return -1;
  }
  // copy over stat content
  memcpy( buf, &response->info, sizeof( struct stat ) );
  *handler = response->handler;
  free( request );
  free( response );
  return 0;
}
