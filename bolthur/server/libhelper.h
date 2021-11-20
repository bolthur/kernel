/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#define USE_DL_PREFIX 1

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <sys/bolthur.h>

#if ! defined( _LIBHELPER_H )
#define _LIBHELPER_H

__weak_symbol void* malloc( size_t n ) {
  void* ret = dlmalloc( n );
  #if defined( HEAP_DEBUG )
    EARLY_STARTUP_PRINT( "malloc = %p with size = %u\r\n", ret, n )
  #endif
  return ret;
}

__weak_symbol void* calloc( size_t size, size_t len ) {
  void* ret = dlcalloc( size, len );
  #if defined( HEAP_DEBUG )
    EARLY_STARTUP_PRINT( "calloc = %p with size = %u and len %u\r\n", ret, size, len )
  #endif
  return ret;
}

__weak_symbol void* realloc( void* p, size_t n ) {
  void* ret = dlrealloc( p, n );
  #if defined( HEAP_DEBUG )
    EARLY_STARTUP_PRINT( "realloc %p = %p with size = %u\r\n", p, ret, n )
  #endif
  return ret;
}

__weak_symbol void free( void* p ) {
  #if defined( HEAP_DEBUG )
    EARLY_STARTUP_PRINT( "free %p\r\n", p )
  #endif
  dlfree( p );
}

/*__weak_symbol void* memalign( size_t alignment, size_t bytes ) {
  void* ret = dlmemalign( alignment, bytes );
  #if defined( HEAP_DEBUG )
    EARLY_STARTUP_PRINT( "memalign %p with alignment = %u and bytes = %u\r\n", ret, alignment, bytes )
  #endif
  return ret;
}*/

/**
 * @fn void send_add_request(vfs_add_request_ptr_t)
 * @brief helper to send add request with wait for response
 *
 * @param msg
 */
__maybe_unused static void send_vfs_add_request( vfs_add_request_ptr_t msg, size_t size ) {
  vfs_add_response_ptr_t response = malloc( sizeof( vfs_add_response_t ) );
  if ( ! response || ! msg ) {
    //EARLY_STARTUP_PRINT( "Allocation failed or invalid message passed!\r\n" )
    exit( -1 );
  }
  // response id
  size_t response_id = 0;
  // try to send until it worked
  do {
    // wait for response
    response_id = _rpc_raise(
      RPC_VFS_ADD,
      VFS_DAEMON_ID,
      ( char* )msg,
      size ? size : sizeof( vfs_add_request_t ),
      true
    );
  } while( errno );
  // erase response
  memset( response, 0, sizeof( vfs_add_response_t ) );
  // get response data
  _rpc_get_data( response, sizeof( vfs_add_response_t ), response_id, false );
  // handle error / no message
  if ( errno ) {
    //EARLY_STARTUP_PRINT( "An error occurred: %s\r\n", strerror( errno ) )
    exit( -1 );
  }
  // stop on success
  if ( VFS_ADD_SUCCESS != response->status ) {
    EARLY_STARTUP_PRINT( "Error while adding %s to vfs!\r\n", msg->file_path )
    exit( -1 );
  }
  // free up response
  free( response );
}

#endif
