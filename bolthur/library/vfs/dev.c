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

#include "dev.h"
#include "add.h"

/**
 * @fn bool add_folder_file(const char*, const uint32_t*, const size_t, const mode_t, const rpc_handler_t)
 * @brief Helper to add a subfolder or file
 *
 * @param path
 * @param device_info
 * @param count
 * @param mode
 * @param handler
 * @return
 */
static bool add_folder_file(
  const char* path,
  const uint32_t* device_info,
  const size_t count,
  const mode_t mode,
  const rpc_handler_t handler
) {
  // allocate memory for add request
  const size_t msg_size = sizeof( vfs_add_request_t ) + count * sizeof( size_t );
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
  vfs_add( msg, msg_size, 0, handler );
  // free stuff
  free( msg );
  // return success
  return true;
}

/**
 * @fn bool vfs_dev_add_folder_file_stat(const char*, const struct st*, const rpc_handler_t)
 * @brief Helper to add a subfolder or file
 *
 * @param path
 * @param st
 * @param handler
 * @return
 */
bool vfs_dev_add_folder_file_stat(
  const char* path,
  const struct stat* st,
  const rpc_handler_t handler
) {
  // allocate memory for add request
  constexpr size_t msg_size = sizeof( vfs_add_request_t ) + 0 * sizeof( size_t );
  vfs_add_request_t* msg = malloc( msg_size );
  if ( ! msg ) {
    return false;
  }
  // clear memory
  memset( msg, 0, msg_size );
  // prepare message structure
  memcpy( &msg->info, st, sizeof( struct stat ) );
  strncpy( msg->file_path, path, PATH_MAX - 1 );
  // perform add request
  vfs_add( msg, msg_size, 0, handler );
  // free stuff
  free( msg );
  return true;
}

/**
 * @fn bool vfs_dev_add_file(const char*, const uint32_t*, size_t, rpc_handler_t)
 * @brief Function to add a device file
 * @param path path to device file
 * @param device_info ioctl commands allowed by file
 * @param count count of ioctl commands in device_info
 * @param handler optional handler for async call flow
 * @return
 */
bool vfs_dev_add_file(
  const char* path,
  const uint32_t* device_info,
  const size_t count,
  const rpc_handler_t handler
) {
  return add_folder_file( path, device_info, count, S_IFCHR, handler );
}

/**
 * @fn bool vfs_dev_add_folder(const char*, const uint32_t*, size_t, rpc_handler_t)
 * @brief Function to add a device file
 * @param path path to device file
 * @param device_info ioctl commands allowed by file
 * @param count count of ioctl commands in device_info
 * @param handler optional handler for async call flow
 * @return
 */
bool vfs_dev_add_folder(
  const char* path,
  const uint32_t* device_info,
  const size_t count,
  const rpc_handler_t handler
) {
  return add_folder_file( path, device_info, count, S_IFCHR /*| S_IFDIR*/, handler );
}
