/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "process.h"
#include "handle.h"

/**
 * @fn void destroy_handle(handle_node_t*)
 * @brief Helper to free up a specific handle
 *
 * @param node
 */
static void destroy_handle( handle_node_t* node ) {
  free( node );
}

/**
 * @fn int generate_handle(process_node_t*)
 * @brief Helper to generate new handle
 *
 * @param process
 * @return
 */
static int generate_handle( process_node_t* process ) {
  return process->handle++;
}

/**
 * @fn int handle_get(handle_node_t**, pid_t, int)
 * @brief Get handle data
 *
 * @param container
 * @param process
 * @param handle
 * @return
 */
int handle_get( handle_node_t** container, pid_t process, int handle ) {
  // generate process container
  process_node_t* process_container = process_generate( process );
  if ( ! process_container ) {
    return -EBADF;
  }
  // create lookup node
  handle_node_t node = { .handle = handle };
  // try to find node
  handle_node_t* found = handle_node_tree_find(
    &process_container->management_tree,
    &node
  );
  // handle not found
  if ( ! found ) {
    return -EBADF;
  }
  // handle found
  *container = found;
  return 0;
}

/**
 * @fn int handle_generate(handle_node_t**, pid_t, pid_t, void*, const char*, int, int)
 * @brief Generate handle structure
 *
 * @param handle
 * @param process
 * @param handler
 * @param data
 * @param path
 * @param flags
 * @param mode
 * @return
 */
int handle_generate(
  handle_node_t** handle,
  pid_t process,
  pid_t handler,
  void* data,
  const char* path,
  int flags,
  int mode
) {
  process_node_t* process_container = process_generate( process );
  if ( ! process_container ) {
    return -ENOMEM;
  }
  // allocate and clear structure
  *handle = malloc( sizeof( handle_node_t ) );
  if ( ! *handle ) {
    return -ENOMEM;
  }
  memset( *handle, 0, sizeof( handle_node_t ) );
  // ensure max path
  if ( PATH_MAX < strlen( path ) ) {
    free( *handle );
    return -EINVAL;
  }
  // copy path
  strncpy( ( *handle )->path, path, PATH_MAX - 1 );
  // special handling for stdin, stdout and stderr
  bool is_stdin = 0 == strcmp( path, "/dev/stdin" );
  bool is_stdout = 0 == strcmp( path, "/dev/stdout" );
  bool is_stderr = 0 == strcmp( path, "/dev/stderr" );
  if ( ! is_stdin && ! is_stdout && ! is_stderr ) {
    ( *handle )->handle = generate_handle( process_container );
  } else {
    if ( is_stdin ) {
      ( *handle )->handle = STDIN_FILENO;
    } else if ( is_stdout ) {
      ( *handle )->handle = STDOUT_FILENO;
    } else if ( is_stderr ) {
      ( *handle )->handle = STDERR_FILENO;
    }

    // check for handle exists to use then the generate method,
    // which should not happen at all
    handle_node_t* tmp_handle_container;
    // try to get handle information
    int tmp_handle_result = handle_get(
      &tmp_handle_container,
      process,
      ( *handle )->handle
    );
    // call to generate handle if return is 0
    if ( 0 == tmp_handle_result ) {
      ( *handle )->handle = generate_handle( process_container );
    }
  }
  // populate structure
  ( *handle )->flags = flags;
  ( *handle )->mode = mode;
  ( *handle )->data = data;
  ( *handle )->handler = handler;
  // insert
  if ( handle_node_tree_insert( &process_container->management_tree, *handle ) ) {
    free( *handle );
    return -EEXIST;
  }
  // return success
  return 0;
}

/**
 * @fn void handle_destory_all(pid_t)
 * @brief Destroy all handles of a process
 *
 * @param process
 */
void handle_destory_all( pid_t process ) {
  // get process container
  process_node_t* process_container = process_generate( process );
  if ( ! process_container ) {
    return;
  }
  // destroy tree
  handle_node_tree_destroy(
    &process_container->management_tree,
    destroy_handle
  );
  // remove from process tree
  process_remove( process_container );
}

/**
 * @brief Destroy handle
 *
 * @param process
 * @param handle
 * @return
 */
int handle_destory( pid_t process, int handle ) {
  // get process container
  process_node_t* process_container = process_generate( process );
  if ( ! process_container ) {
    return -EBADF;
  }
  // create lookup node
  handle_node_t node = { .handle = handle };
  // try to find node
  handle_node_t* found = handle_node_tree_find(
    &process_container->management_tree,
    &node
  );
  // handle not found
  if ( ! found ) {
    return -EBADF;
  }
  // remove node from tree
  handle_node_tree_remove(
    &process_container->management_tree,
    found
  );
  // free up found
  free( found );
  // return success
  return 0;
}
