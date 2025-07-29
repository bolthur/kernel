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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "handler.h"
#include "../../../../../library/collection/avl/avl.h"

static avl_tree_t* management_tree;


/**
 * @fn int32_t compare_container(const avl_node_t*, const avl_node_t*)
 * @brief Compare handle callback necessary for avl tree insert / delete
 *
 * @param node_a
 * @param node_b
 * @return
 */
static int32_t compare_container(
  const avl_node_t* node_a,
  const avl_node_t* node_b
) {
  const pid_container_t* container_a = PID_HANDLER_GET_ENTRY( node_a );
  const pid_container_t* container_b = PID_HANDLER_GET_ENTRY( node_b );
  // return 0 if equal
  if ( container_a->handler == container_b->handler ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container_a->handler > container_b->handler ? -1 : 1;
}

/**
 * @fn int32_t lookup_container(const avl_node_t*, const void*)
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node
 * @param value
 * @return
 */
static int32_t lookup_container(
  const avl_node_t* node,
  const void* value
) {
  pid_t handler = ( pid_t )value;
  pid_container_t* container = PID_HANDLER_GET_ENTRY( node );
  // return 0 if equal
  if ( container->handler == handler ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container->handler > handler ? -1 : 1;
}

/**
 * @fn void cleanup_container(avl_node_t*)
 * @brief handle cleanup
 *
 * @param node
 */
static void cleanup_container( avl_node_t* node ) {
  pid_container_t* item = PID_HANDLER_GET_ENTRY( node );
  // free item
  free( item );
}

/**
 * @fn int handler_init(void)
 * @brief Wrapper to init handler management
 * @return
 */
int handler_init( void ) {
  // generate tree
  management_tree = avl_create_tree(
    compare_container,
    lookup_container,
    cleanup_container
  );
  // handle error
  if ( ! management_tree ) {
    return ENOMEM;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_register(libhid_interface_type_t, pid_t)
 * @brief Method to register a handler
 * @param type
 * @param handler
 * @return
 */
int handler_register( const libhid_interface_type_t type, const pid_t handler ) {
  // validate
  if ( ! management_tree ) {
    return EINVAL;
  }
  // try to find possible handler
  const avl_node_t* found = avl_find_by_data( management_tree, ( void* )handler );
  // handle found
  if ( found ) {
    return EINVAL;
  }
  // allocate new container
  pid_container_t* item = malloc( sizeof( *item ) );
  if ( ! item ) {
    return ENOMEM;
  }
  // clear out
  memset( item, 0, sizeof( *item ) );
  // pure in data
  item->handler = handler;
  // prepare node
  avl_prepare_node( &item->node, ( void* )type );
  // insert into tree
  if ( ! avl_insert_by_node( management_tree, &item->node ) ) {
    free( item );
    return EAGAIN;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_unregister(libhid_interface_type_t, pid_t)
 * @brief Unregister a handler
 * @param type
 * @param handler
 * @return
 */
int handler_unregister( const libhid_interface_type_t type, const pid_t handler ) {
  // validate
  if ( ! management_tree ) {
    return EINVAL;
  }
  // try to find possible handler
  avl_node_t* found = avl_find_by_data( management_tree, ( void* )type );
  // handle not found => return success
  if ( ! found ) {
    return 0;
  }
  // compare handlers
  pid_container_t* item = PID_HANDLER_GET_ENTRY( found );
  // handle no matcj
  if ( item->handler != handler ) {
    return EINVAL;
  }
  // return node
  avl_remove_by_node( management_tree, found );
  // cleanup node
  free( item );
  // return success
  return 0;
}

/**
 * @fn int handler_get(libhid_interface_type_t, pid_t*)
 * @brief Get a handler
 * @param type
 * @param handler
 * @return
 */
int handler_get( const libhid_interface_type_t type, pid_t* handler ) {
  // validate
  if ( ! handler || ! management_tree ) {
    return EINVAL;
  }
  // try to find possible handler
  const avl_node_t* found = avl_find_by_data( management_tree, ( void* )type );
  // handle not found
  if ( ! found ) {
    *handler = 0;
  // handle found
  } else {
    pid_container_t* container = PID_HANDLER_GET_ENTRY( found );
    *handler = container->handler;
  }
  // return success
  return 0;
}
