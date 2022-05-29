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

#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "handler.h"

static avl_tree_t* ioctl_tree = NULL;

/**
 * @fn int32_t compare_ioctl(const avl_node_t*, const avl_node_t*)
 * @brief Compare handle callback necessary for avl tree insert / delete
 *
 * @param node_a
 * @param node_b
 * @return
 */
static int32_t compare_ioctl(
  const avl_node_t* node_a,
  const avl_node_t* node_b
) {
  ioctl_tree_entry_t* container_a = IOCTL_HANDLER_GET_ENTRY( node_a );
  ioctl_tree_entry_t* container_b = IOCTL_HANDLER_GET_ENTRY( node_b );
  // return 0 if equal
  if ( container_a->pid == container_b->pid ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container_a->pid > container_b->pid ? -1 : 1;
}

/**
 * @fn int32_t lookup_ioctl(const avl_node_t*, const void*)
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node
 * @param value
 * @return
 */
static int32_t lookup_ioctl(
  const avl_node_t* node,
  const void* value
) {
  pid_t pid = ( pid_t )value;
  ioctl_tree_entry_t* container = IOCTL_HANDLER_GET_ENTRY( node );
  // return 0 if equal
  if ( container->pid == pid ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container->pid > pid ? -1 : 1;
}

/**
 * @fn void cleanup_ioctl(avl_node_t*)
 * @brief handle cleanup
 *
 * @param node
 */
static void cleanup_ioctl( avl_node_t* node ) {
  ioctl_tree_entry_t* item = IOCTL_HANDLER_GET_ENTRY( node );
  // destroy tree
  avl_destroy_tree( item->tree );
  // free item
  free( item );
}

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
  ioctl_container_t* container_a = IOCTL_HANDLER_GET_CONTAINER( node_a );
  ioctl_container_t* container_b = IOCTL_HANDLER_GET_CONTAINER( node_b );
  // return 0 if equal
  if ( container_a->command == container_b->command ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container_a->command > container_b->command ? -1 : 1;
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
  uint32_t command = ( uint32_t )value;
  ioctl_container_t* container = IOCTL_HANDLER_GET_CONTAINER( node );
  // return 0 if equal
  if ( container->command == command ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container->command > command ? -1 : 1;
}

/**
 * @fn void cleanup_container(avl_node_t*)
 * @brief handle cleanup
 *
 * @param node
 */
static void cleanup_container( avl_node_t* node ) {
  ioctl_container_t* item = IOCTL_HANDLER_GET_CONTAINER( node );
  // free item
  free( item );
}

/**
* @fn bool ioctl_handler_init(void)
 * @brief Initialize ioct handler
 *
 * @return
 */
bool ioctl_handler_init( void ) {
  // create handle tree
  ioctl_tree = avl_create_tree(
    compare_ioctl,
    lookup_ioctl,
    cleanup_ioctl
  );
  return ioctl_tree;
}

/**
 * @fn ioctl_container_t* ioctl_lookup_command(uint32_t, pid_t)
 * @brief Helper for command lookup
 *
 * @param command
 * @param handle
 * @return
 */
ioctl_container_t* ioctl_lookup_command(
  uint32_t command,
  pid_t process
) {
  // try to find command within tree before querying info
  avl_node_t* found = avl_find_by_data(
    ioctl_tree,
    ( void* )process
  );
  // handle nothing found
  if ( ! found ) {
    return NULL;
  }
  // get entry
  ioctl_tree_entry_t* entry = IOCTL_HANDLER_GET_ENTRY( found );
  // lookup command
  found = avl_find_by_data(
    entry->tree,
    ( void* )command
  );
  // handle nothing found
  if ( ! found ) {
    return NULL;
  }
  // return found entry
  return IOCTL_HANDLER_GET_CONTAINER( found );
}

/**
 * @fn bool ioctl_push_command(uint32_t, pid_t)
 * @brief Push command for process
 *
 * @param command
 * @param handle
 * @return
 */
bool ioctl_push_command(
  uint32_t command,
  pid_t process
) {
  // treat existing commands already pushed / success
  if ( ioctl_lookup_command( command, process ) ) {
    return true;
  }
  // try to find command within tree before querying info
  avl_node_t* found = avl_find_by_data(
    ioctl_tree,
    ( void* )process
  );
  ioctl_tree_entry_t* entry = NULL;
  // handle existing tree
  if ( found ) {
    // get entry
    entry = IOCTL_HANDLER_GET_ENTRY( found );
  } else {
    entry = malloc( sizeof( ioctl_tree_entry_t ) );
    if ( ! entry ) {
      return false;
    }
    memset( entry, 0, sizeof( ioctl_tree_entry_t ) );
    entry->pid = process;
    // FIXME: ADD CORRECT TREE CALLBACKS
    entry->tree = avl_create_tree(
      compare_container,
      lookup_container,
      cleanup_container
    );
    if ( ! entry->tree ) {
      free( entry );
      return false;
    }
    // prepare and insert node
    avl_prepare_node( &entry->node, ( void* )process );
    if ( ! avl_insert_by_node( ioctl_tree, &entry->node ) ) {
      avl_destroy_tree( entry->tree );
      free( entry );
      return false;
    }
  }
  // allocate container
  ioctl_container_t* container = malloc( sizeof( ioctl_container_t ) );
  if ( ! container ) {
    return false;
  }
  // fill container
  memset( container, 0, sizeof( ioctl_container_t ) );
  container->command = command;
  // prepare and insert node
  avl_prepare_node( &container->node, ( void* )command );
  if ( ! avl_insert_by_node( entry->tree, &container->node ) ) {
    free( container );
    return false;
  }
  // return created container
  return true;
}
