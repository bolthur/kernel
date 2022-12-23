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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/bolthur.h>

#if ! defined( _LIBHELPER_H )
#define _LIBHELPER_H

/**
 * @fn void send_vfs_add_request(vfs_add_request_t*, size_t, unsigned int)
 * @brief Helper to send add request with wait for response
 *
 * @param msg message to send
 * @param size message size or 0
 * @param wait amount of seconds to sleep on rpc raise error
 */
__maybe_unused static void send_vfs_add_request(
  vfs_add_request_t* msg,
  size_t size,
  unsigned int wait
) {
  vfs_add_response_t* response = malloc( sizeof( *response ) );
  if ( ! response || ! msg ) {
    exit( -1 );
  }
  // push in current pid
  msg->handler = getpid();
  size_t size_to_use = size ? size : sizeof( *msg );
  // response id
  size_t response_id = 0;
  // try to send until it worked
  while ( true ) {
    // wait for response
    response_id = bolthur_rpc_raise(
      RPC_VFS_ADD,
      VFS_DAEMON_ID,
      msg,
      size_to_use,
      NULL,
      RPC_VFS_ADD,
      msg,
      size_to_use,
      0,
      0
    );
    if ( errno ) {
      if ( wait ) {
        sleep( wait );
      }
      continue;
    }
    break;
  }
  // erase response
  memset( response, 0, sizeof( *response ) );
  // get response data
  _syscall_rpc_get_data(
    response,
    sizeof( *response ),
    response_id,
    false
  );
  // handle error / no message
  if ( errno ) {
    exit( -1 );
  }
  // stop on success
  if ( VFS_ADD_SUCCESS != response->status ) {
    exit( -1 );
  }
  // free up response
  free( response );
}

/**
 * @fn void send_vfs_add_request(vfs_add_request_t*, size_t, unsigned int)
 * @brief Helper to send add request with wait for response
 *
 * @param msg message to send
 * @param size message size or 0
 * @param wait amount of seconds to sleep on rpc raise error
 */
__maybe_unused static void send_vfs_remove_request(
  vfs_remove_request_t* msg,
  unsigned int wait
) {
  vfs_remove_response_t* response = malloc( sizeof( *response ) );
  if ( ! response || ! msg ) {
    exit( -1 );
  }
  // response id
  size_t response_id = 0;
  // try to send until it worked
  while ( true ) {
    // wait for response
    response_id = bolthur_rpc_raise(
      RPC_VFS_REMOVE,
      VFS_DAEMON_ID,
      msg,
      sizeof( *msg ),
      NULL,
      RPC_VFS_ADD,
      msg,
      sizeof( *msg ),
      0,
      0
    );
    if ( errno ) {
      if ( wait ) {
        sleep( wait );
      }
      continue;
    }
    break;
  }
  // erase response
  memset( response, 0, sizeof( *response ) );
  // get response data
  _syscall_rpc_get_data( response, sizeof( *response ), response_id, false );
  // handle error / no message
  if ( errno ) {
    exit( -1 );
  }
  // stop on success
  if ( 0 != response->status ) {
    exit( -1 );
  }
  // free up response
  free( response );
}

/**
 * @fn void wait_for_path(const char*)
 * @brief Wait for vfs path is existing
 *
 * @param path
 */
__maybe_unused static void vfs_wait_for_path( const char* path ) {
  struct stat buffer;
  do {
    sleep( 2 );
  } while( 0 != stat( path, &buffer ) );
}

/**
 * @fn bool dev_add_folder_file(const char*, uint32_t*, size_t, mode_t)
 * @brief Helper to add a subfolder or file
 *
 * @param path
 * @param device_info
 * @param count
 * @param mode
 * @return
 */
__maybe_unused static bool dev_add_folder_file(
  const char* path,
  uint32_t* device_info,
  size_t count,
  mode_t mode
) {
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + count * sizeof( size_t );
  vfs_add_request_t* msg = malloc( msg_size );
  if ( ! msg ) {
    return false;
  }
  // clear memory
  memset( msg, 0, msg_size );
  // debug output
  EARLY_STARTUP_PRINT( "Sending \"%s\" to vfs\r\n", path )
  // prepare message structure
  msg->info.st_mode = mode;
  strncpy( msg->file_path, path, PATH_MAX - 1 );
  // copy over device info stuff
  if ( device_info ) {
    for ( size_t idx = 0; idx < count; idx++ ) {
      msg->device_info[ idx ] = device_info[ idx ];
    }
  }
  // perform add request
  send_vfs_add_request( msg, msg_size, 0 );
  // free stuff
  free( msg );
  return true;
}

/**
 * @fn bool dev_add_file(const char*, uint32_t*, size_t)
 * @brief Wrapper to add a file
 *
 * @param path
 * @param device_info
 * @param count
 * @return
 */
__maybe_unused static bool dev_add_file(
  const char* path,
  uint32_t* device_info,
  size_t count
) {
  return dev_add_folder_file( path, device_info, count, S_IFCHR );
}

/**
 * @fn bool dev_add_folder(const char*, uint32_t*, size_t)
 * @brief Wrapper to add a folder
 *
 * @param path
 * @param device_info
 * @param count
 * @return
 */
__maybe_unused static bool dev_add_folder(
  const char* path,
  uint32_t* device_info,
  size_t count
) {
  return dev_add_folder_file( path, device_info, count, S_IFCHR /*| S_IFDIR*/ );
}

#endif
