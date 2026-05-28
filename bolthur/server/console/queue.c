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

#include <stddef.h>
#include "queue.h"

static queue_head_t management_queue;

/**
 * @fn bool queue_setup(void)
 * @brief queue setup
 *
 * @return
 */
bool queue_setup( void ) {
  TAILQ_INIT( &management_queue );
  return true;
}

/**
 * @fn bool queue_push(size_t, size_t, vfs_read_request_t*, handler_node_t*)
 * @brief
 * @param type
 * @param response_info
 * @param request
 * @param node
 * @return
 */
bool queue_push( const size_t type, const size_t response_info, vfs_read_request_t* request, handler_node_t* node )
{
  // allocate node
  queue_node_t* e = malloc( sizeof( queue_node_t ) );
  if ( ! e ) {
    return false;
  }
  // clear space
  memset( e, 0, sizeof( queue_node_t ) );
  // populate response and request
  e->response_info = response_info;
  e->request = request;
  e->handler = node;
  e->return_type = type;
  TAILQ_INSERT_TAIL( &management_queue, e, node);
  // return success
  return true;
}

/**
 * @fn bool queue_handle(const char*, const char*)
 * @brief
 * @param file file
 * @param data data to process
 * @return
 */
void queue_handle( const char* file, const char* data ) {
  queue_node_t* e = NULL;
  queue_node_t* next = NULL;
  TAILQ_FOREACH_SAFE( e, &management_queue, node, next ) {
    // handle path match and active console
    if ( e->handler->console->active && 0 == strcmp( e->request->file_path, file ) ) {
      char* area = _syscall_memory_shared_attach( e->request->shm_id, ( uintptr_t )NULL );
      if ( area ) {
        // get total len and evaluate to read
        const size_t len = strlen( data );
        size_t to_read = len;
        EARLY_STARTUP_PRINT( "e->request->len = %zu\r\n", e->request->len )
        if ( to_read > e->request->len - e->read_amount ) {
          to_read = e->request->len - e->read_amount;
        }
        // check for newline
        const char* newline = strstr( data, "\r\n" );
        // subtract carriage return and newline
        if ( newline ) {
          to_read -= 2;
        }
        // copy over if there is something
        if ( to_read ) {
          memcpy( area + e->read_amount, data, to_read );
        }
        // increment read amount
        e->read_amount += to_read;
        // handle newline by set fulfilled
        if ( newline ) {
          ( area + e->read_amount )[ 0 ] = '\n';
          ( area + e->read_amount )[ 1 ] = '\0';
          e->read_amount += 2;
          e->request->len = e->read_amount;
        } else {
          ( area + e->read_amount )[ 0 ] = '\0';
        }
        // detach again
        _syscall_memory_shared_detach( e->request->shm_id );
        // handle finished
        if ( e->read_amount >= e->request->len ) {
          // return to process
          vfs_read_response_t response = { .len = ( ssize_t )e->read_amount };
          bolthur_rpc_return(
            e->return_type,
            &response,
            sizeof( response ),
            NULL,
            e->response_info
          );
          // remove from node
          TAILQ_REMOVE(&management_queue, e, node);
          // free up request and queue entry
          free( e->request );
          free( e );
        }
      }
    }
  }
}

/**
 * @fn void queue_cleanup(const console_t*)
 * @brief Cleanup queued input handlers for console
 * @param console console to be cleaned up
 */
void queue_cleanup( const console_t* console ) {
  queue_node_t* e = NULL;
  queue_node_t* next = NULL;
  TAILQ_FOREACH_SAFE( e, &management_queue, node, next ) {
    // handle path match and active console
    if ( e->handler->console == console ) {
      // remove from node
      TAILQ_REMOVE(&management_queue, e, node);
      // free up request and queue entry
      free( e->request );
      free( e );
    }
  }
}
