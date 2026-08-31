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

#include <errno.h>
#include <string.h>
#include <sys/bolthur.h>
#include "handler.h"

static list_manager_t* management_list;

/**
 * @fn int32_t lookup_container(const avl_node_t*, const void*)
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node
 * @param value
 * @return
 */
static int32_t lookup_container(
  const list_item_t* node,
  const void* value
) {
  auto const type = ( pid_t )value;
  auto const node_type = ( pid_t )node->data;
  // return 0 if equal
  if ( node_type == type ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return node_type > type ? -1 : 1;
}

/**
 * @fn int handler_init(void)
 * @brief Wrapper to init handler management
 * @return
 */
int handler_init( void ) {
  // generate tree
  management_list = list_construct(
    lookup_container,
    nullptr,
    nullptr
  );
  // handle error
  if ( ! management_list ) {
    return ENOMEM;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_register(pid_t)
 * @brief Method to register a handler
 * @param handler
 * @return
 */
int handler_register( const pid_t handler ) {
  // validate
  if ( ! management_list || ! handler ) {
    return EINVAL;
  }
  // try to find
  const list_item_t* found = list_lookup_data( management_list, ( void* )handler );
  // handle found
  if ( found ) {
    return EINVAL;
  }
  // insert into tree
  if ( ! list_insert_data( management_list, ( void* )handler ) ) {
    return EAGAIN;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_unregister(pid_t)
 * @brief Unregister a handler
 * @param handler
 * @return
 */
int handler_unregister( const pid_t handler ) {
  // validate
  if ( ! management_list || ! handler ) {
    return EINVAL;
  }
  // try to find
  list_item_t* found = list_lookup_data( management_list, ( void* )handler );
  // handle found
  if ( ! found ) {
    return 0;
  }
  // remove data
  if ( ! list_remove_item( management_list, found, true ) ) {
    return EAGAIN;
  }
  // return success
  return 0;
}

/**
 * @fn list_item_t* handler_first(void)
 * @brief Get first handler item
 * @return
 */
list_item_t* handler_first( void ) {
  if ( ! management_list ) {
    return nullptr;
  }
  return management_list->first;
}
