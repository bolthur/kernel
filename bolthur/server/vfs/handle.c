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

#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "handle.h"
#include "avl.h"

/**
 * @brief tree managing file handles
 */
avl_tree_ptr_t handle_process_tree = NULL;

/**
 * @brief Compare handle callback necessary for avl tree insert / delete
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_handle(
  const avl_node_ptr_t node_a,
  const avl_node_ptr_t node_b
) {
  handle_container_ptr_t container_a = HANDLE_GET_CONTAINER( node_a );
  handle_container_ptr_t container_b = HANDLE_GET_CONTAINER( node_b );
  // return 0 if equal
  if ( container_a->handle == container_b->handle ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container_a->handle > container_b->handle ? -1 : 1;
}

/**
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node current node
 * @param value value that is looked up
 * @return int32_t
 */
static int32_t lookup_handle(
  const avl_node_ptr_t node,
  const void* value
) {
  int handle = ( int )value;
  handle_container_ptr_t container = HANDLE_GET_CONTAINER( node );
  // return 0 if equal
  if ( container->handle == handle ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container->handle > handle ? -1 : 1;
}

/**
 * @brief Compare handle callback necessary for avl tree insert / delete
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_process(
  const avl_node_ptr_t node_a,
  const avl_node_ptr_t node_b
) {
  handle_pid_ptr_t container_a = HANDLE_GET_PID( node_a );
  handle_pid_ptr_t container_b = HANDLE_GET_PID( node_b );
  // return 0 if equal
  if ( container_a->pid == container_b->pid ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container_a->pid > container_b->pid ? -1 : 1;
}

/**
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node current node
 * @param value value that is looked up
 * @return int32_t
 */
static int32_t lookup_process(
  const avl_node_ptr_t node,
  const void* value
) {
  pid_t pid = ( int )value;
  handle_pid_ptr_t container = HANDLE_GET_PID( node );
  // return 0 if equal
  if ( container->pid == pid ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return container->pid > pid ? -1 : 1;
}

/**
 * @brief Method to generate next handle
 *
 * @return
 */
static int generate_handle( handle_pid_ptr_t handle ) {
  return handle->handle++;
}

/**
 * @brief Init file handle tree
 *
 * @todo build handle tree per pid
 */
bool handle_init( void ) {
  // create handle tree
  handle_process_tree = avl_create_tree( compare_process, lookup_process, NULL );
  return handle_process_tree;
}

/**
 * @brief Generate new handle
 *
 * @param container
 * @param process
 * @param target
 * @param parent
 * @param flags
 * @param mode
 * @return
 *
 * @todo return negative error codes instead of -1
 */
int handle_generate(
  handle_container_ptr_t* container,
  pid_t process,
  __unused vfs_node_ptr_t parent,
  vfs_node_ptr_t target,
  const char* path,
  int flags,
  int mode
) {
  handle_pid_ptr_t process_handle;
  // get handle tree
  avl_node_ptr_t found = avl_find_by_data(
    handle_process_tree,
    ( void* )process );
  // create if not existing
  if ( ! found ) {
    // allocate process handle
    process_handle = malloc( sizeof( handle_pid_t ) );
    if ( ! process_handle ) {
      return -1;
    }
    // clear
    memset( process_handle, 0, sizeof( handle_pid_t ) );
    // populate structure
    process_handle->pid = process;
    process_handle->handle = 3; // handles start at 3, everything below is reserved
    process_handle->tree = avl_create_tree( compare_handle, lookup_handle, NULL );
    // handle error
    if ( ! process_handle->tree ) {
      free( process_handle );
      return -1;
    }
    // prepare and insert node
    avl_prepare_node( &process_handle->node, ( void* )process );
    if ( ! avl_insert_by_node( handle_process_tree, &process_handle->node ) ) {
      avl_destroy_tree( process_handle->tree );
      free( process_handle );
      return -1;
    }
  // get process handle if found
  } else {
    process_handle = HANDLE_GET_PID( found );
  }
  // allocate and clear structure
  *container = malloc( sizeof( handle_container_t ) );
  if ( ! *container ) {
    return -1;
  }
  memset( *container, 0, sizeof( handle_container_t ) );
  // copy path
  strcpy( ( *container )->path, path );
  // special handling for symlinks
  if ( S_ISLNK( target->st->st_mode ) ) {
    while ( target && S_ISLNK( target->st->st_mode ) ) {
      if ( '/' == target->target[ 0 ] ) {
        target = vfs_node_by_path( target->target );
      } else {
        // allocate target
        char* link_path = malloc( sizeof( char ) * PATH_MAX );
        if ( ! link_path ) {
          free( *container );
          return -1;
        }
        char* target_path = vfs_path_bottom_up( target );
        if ( ! target_path ) {
          free( link_path );
          free( *container );
          return -1;
        }
        // dir
        char* dir = dirname( target_path );
        if ( ! dir ) {
          free( target_path );
          free( link_path );
          free( *container );
          return -1;
        }
        // build target path
        link_path = strncpy( link_path, dir, PATH_MAX );
        link_path = strncat( link_path, "/", PATH_MAX - strlen( link_path ) );
        link_path = strncat( link_path, target->target, PATH_MAX - strlen( link_path ) );
        // free up base path again
        free( target_path );
        free( dir );
        // overwrite target
        target = vfs_node_by_path( link_path );
        // free built link path
        free( link_path );
      }
    }
    // get link destination
    char* destination = vfs_path_bottom_up( target ) ;
    if ( ! destination ) {
      free( *container );
      return -1;
    }
    // copy link destination
    strcpy( ( *container )->path, destination );
    // free again after copy
    free( destination );
  }
  // populate structure
  ( *container )->flags = flags;
  ( *container )->mode = mode;
  ( *container )->target = target;
  ( *container )->handle = generate_handle( process_handle );
  // prepare and insert avl node
  avl_prepare_node( &(*container)->node, ( void* )(*container)->handle );
  if ( ! avl_insert_by_node( process_handle->tree, &(*container)->node ) ) {
    free( *container );
    return -1;
  }
  // return success
  return 0;
}

/**
 * @brief Destroy handle
 *
 * @param process
 * @param handle
 * @return
 */
int handle_destory( pid_t process, int handle ) {
  handle_pid_ptr_t process_handle;
  // get handle tree
  avl_node_ptr_t found = avl_find_by_data(
    handle_process_tree,
    ( void* )process );
  // create if not existing
  if ( ! found ) {
    return -EBADF;
  }
  // get pid handle container
  process_handle = HANDLE_GET_PID( found );
  // lookup handle itself
  found = avl_find_by_data(
    process_handle->tree,
    ( void* )handle );
  // handle not found
  if ( ! found ) {
    return -EBADF;
  }
  // get structure
  handle_container_ptr_t container = HANDLE_GET_CONTAINER( found );
  // remove from tree
  avl_remove_by_node( process_handle->tree, &container->node );
  // free data
  free( container );
  // return success
  return 0;
}

/**
 * @brief get handle data
 *
 * @param container
 * @param process
 * @param handle
 * @return
 */
int handle_get( handle_container_ptr_t* container, pid_t process, int handle ) {
  handle_pid_ptr_t process_handle;
  // get handle tree
  avl_node_ptr_t found = avl_find_by_data(
    handle_process_tree,
    ( void* )process );
  // create if not existing
  if ( ! found ) {
    return -EBADF;
  }
  // get process handle if found
  process_handle = HANDLE_GET_PID( found );
  // lookup handle itself
  found = avl_find_by_data(
    process_handle->tree,
    ( void* )handle );
  // handle not found
  if ( ! found ) {
    return -EBADF;
  }
  *container = HANDLE_GET_CONTAINER( found );
  return 0;
}
