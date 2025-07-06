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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/bolthur.h>
#include "configuration.h"
#include "util.h"

// initialize list
TAILQ_HEAD(head_s, configuration_node) head;

/**
 * @fn int configuration_confini_handler(IniDispatch*, void*)
 * @brief Handler used for parsing file
 *
 * @param dispatch
 * @param v_null
 * @return
 */
int configuration_confini_handler (
  IniDispatch* dispatch,
  [[maybe_unused]] void* v_null
) {
  // determine section information
  const char* section = dispatch->append_to;
  if ( INI_SECTION == dispatch->type ) {
    section = dispatch->data;
  }
  // get list item
  configuration_node_t* n = by_name( section );
  // create if not found
  if ( ! n ) {
    // allocate node
    n = malloc( sizeof( *n ) );
    if ( ! n ) {
      return 1;
    }
    // clear out
    memset( n, 0, sizeof( *n ) );
    // copy name
    strcpy( n->name, section );
    // push to list
    TAILQ_INSERT_TAIL( &head, n, queue );
  }
  // handle early end
  if ( INI_SECTION == dispatch->type ) {
    return 0;
  }
  // name and value
  const char* name = dispatch->data;
  const char* value = dispatch->value;
  // push back data
  if ( 0 == strcmp( name, "path") ) {
    // copy path
    strcpy( n->path, value );
  } else if ( 0 == strcmp( name, "device") ) {
    // copy path
    strcpy( n->device, value );
  } else if ( 0 == strcmp( name, "early") ) {
    n->early = 0 == strcmp( value, "true" );
  } else if ( 0 == strcmp( name, "reroute") ) {
    n->reroute = 0 == strcmp( value, "true" );
  } else {
    EARLY_STARTUP_PRINT(
      "unknown key \"%s\" in section \"%s\"\r\n", name, section )
    return 1;
  }
  return 0;
}

/**
 * @fn node_t by_name*(const char*)
 * @brief Get node by name
 *
 * @param name
 * @return
 */
configuration_node_t* by_name( const char* name ) {
  // variable
  configuration_node_t* e = NULL;
  // loop through list and try to find by name
  TAILQ_FOREACH(e, &head, queue) {
    if (
      strlen( name ) == strlen( e->name )
      && 0 == strcmp( name, e->name )
    ) {
      return e;
    }
  }
  // not found
  return NULL;
}

/**
 * @fn bool configuration_handle(const char*)
 * @brief Parse and handle configuration
 *
 * @param path
 * @return
 */
bool configuration_handle( const char* path ) {
  // init list
  TAILQ_INIT(&head);
  // open file
  FILE* ini_file = fopen( path, "rb" );
  if ( ! ini_file ) {
    EARLY_STARTUP_PRINT( "Unable to open test ini file\r\n" )
    return false;
  }

  // load ini file
  if ( load_ini_file(
    ini_file,
    INI_DEFAULT_FORMAT,
    NULL,
    configuration_confini_handler,
    NULL
  ) ) {
    EARLY_STARTUP_PRINT( "Cannot load ini file!\r\n" )
    return false;
  }
  // loop through queue and print
  configuration_node_t* n;
  TAILQ_FOREACH(n, &head, queue) {
    // output
    if ( n->early ) {
      EARLY_STARTUP_PRINT( "Starting server %s...\r\n", n->name )
    } else {
      STARTUP_PRINT( "Starting server %s...\r\n", n->name )
    }
    // start server
    util_execute_device_server( n->path, n->device );
    // reroute handling
    if ( n->reroute ) {
      // ORDER NECESSARY HERE DUE TO THE DEFINES
      // reroute stdin
      EARLY_STARTUP_PRINT( "Rerouting stdin, stdout and stderr\r\n" )
      FILE* fpin = freopen( "/dev/stdin", "r", stdin );
      if ( ! fpin ) {
        EARLY_STARTUP_PRINT( "Unable to reroute stdin\r\n" )
        exit( 1 );
      }
      // reroute stdout
      EARLY_STARTUP_PRINT( "stdin fileno = %d\r\n", fpin->_file )
      FILE* fpout = freopen( "/dev/stdout", "w", stdout );
      if ( ! fpout ) {
        EARLY_STARTUP_PRINT( "Unable to reroute stdout\r\n" )
        exit( 1 );
      }
      // reroute stderr
      EARLY_STARTUP_PRINT( "stdout fileno = %d\r\n", fpout->_file )
      FILE* fperr = freopen( "/dev/stderr", "w", stderr );
      if ( ! fperr ) {
        EARLY_STARTUP_PRINT( "Unable to reroute stderr\r\n" )
        exit( 1 );
      }
      EARLY_STARTUP_PRINT( "stderr fileno = %d\r\n", fperr->_file )
      // allocate request
      vfs_boot_init_request_t* request = malloc( sizeof( vfs_boot_init_request_t ) );
      // handle request error
      if ( ! request ) {
        EARLY_STARTUP_PRINT( "Unable to allocate memory\r\n" )
        exit( 1 );
      }
      // clear and prepare memory
      memset( request, 0, sizeof( *request ) );
      strncpy( request->in, "/dev/stdin", PATH_MAX );
      strncpy( request->out, "/dev/stdout", PATH_MAX );
      strncpy( request->err, "/dev/stderr", PATH_MAX );
      // wait for response
      size_t response_id = bolthur_rpc_raise(
        RPC_VFS_BOOT_INIT,
        VFS_DAEMON_ID,
        request,
        sizeof( *request ),
        NULL,
        RPC_VFS_BOOT_INIT,
        request,
        sizeof( *request ),
        0,
        0,
        NULL
      );
      if ( errno ) {
        EARLY_STARTUP_PRINT( "Unable to call boot init: %s\r\n", strerror( errno ) )
        exit( 1 );
      }
      // free request again
      free( request );
      // get message and data size
      size_t data_size;
      vfs_boot_init_response_t* response = bolthur_rpc_fetch_from_mailbox(
        response_id, &data_size, true, NULL );
      if ( errno ) {
        EARLY_STARTUP_PRINT( "Unable to fetch boot init response: %s\r\n", strerror(errno) )
        exit( -1 );
      }
      // handle success not one
      if (response->result != 0) {
        EARLY_STARTUP_PRINT( "Boot init request failed!\r\n" )
        free( response );
        exit( -1 );
      }
      // free response again
      free( response );
    }
  }
  // free queue again
  while ( ! TAILQ_EMPTY( &head ) ) {
    // get first node
    n = TAILQ_FIRST( &head );
    // remove from queue
    TAILQ_REMOVE( &head, n, queue );
    // free data
    free( n );
    // set pointer to null
    n = NULL;
  }
  return 0;
}
