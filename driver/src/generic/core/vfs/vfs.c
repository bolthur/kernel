
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/bolthur.h>

#include "list.h"
#include "vfs.h"
#include "util.h"

/**
 * @brief helper to destroy a node
 *
 * @param node
 */
static void vfs_destroy_node( vfs_node_ptr_t node ) {
  if ( ! node ) {
    return;
  }
  if ( node->pid ) {
    free( node->pid );
  }
  if ( node->name ) {
    free( node->name );
  }
  if ( node->children ) {
    list_destruct( node->children );
  }
  free( node );
}

/**
 * @brief Create and prepare vfs node
 *
 * @param pid
 * @param name
 * @param flags
 * @param parent
 * @return
 */
static vfs_node_ptr_t vfs_prepare_node(
  pid_t pid,
  const char* name,
  uint32_t flags,
  vfs_node_ptr_t parent
) {
  // allocate and error handling
  vfs_node_ptr_t node = malloc( sizeof( vfs_node_t ) );
  if ( ! node ) {
    return NULL;
  }
  // erase memory
  memset( node, 0, sizeof( vfs_node_t ) );
  // allocate name
  node->name = malloc( sizeof( char ) * ( strlen( name ) + 1 ) );
  if ( ! node->name ) {
    vfs_destroy_node( node );
    return NULL;
  }
  // allocate pid
  node->pid = malloc( sizeof( pid_t ) );
  if ( ! node->pid ) {
    vfs_destroy_node( node );
    return NULL;
  }
  // copy name, save current pid as handler and set flag to directory
  strcpy( node->name, name );
  *( node->pid ) = pid;
  node->flags = flags;
  // create list
  // FIXME: Add lookup and cleanup functions
  node->children = list_construct( NULL, NULL );
  if ( ! node->children ) {
    vfs_destroy_node( node );
    return NULL;
  }
  // append to parent
  if ( parent ) {
    // set parent
    node->parent = parent;
    // push dev item to root list
    if ( ! list_push_back( parent->children, node ) ) {
      vfs_destroy_node( node );
      return NULL;
    }
  }
  // return constructed node
  return node;
}

/**
 * @brief add node by path parts
 *
 * @param part
 * @param parent
 * @return
 */
static vfs_node_ptr_t vfs_node_by_path_part(
  const char* part,
  vfs_node_ptr_t parent
) {
  // get first item
  list_item_ptr_t start = parent->children->first;
  if ( ! start ) {
    return NULL;
  }
  // loop till end
  while ( start ) {
    vfs_node_ptr_t node = ( vfs_node_ptr_t )start->data;
    if ( ! node ) {
      continue;
    }
    // check for path part
    if ( 0 == strcmp( part, node->name ) ) {
      return node;
    }
    // next one
    start = start->next;
  }
  return NULL;
}

/**
 * @brief method to destroy node
 *
 * @param node
 */
void vfs_destroy( vfs_node_ptr_t node ) {
  // handle invalid value
  if ( ! node ) {
    return;
  }

  // handle children destroy first
  if ( node->children ) {
    list_item_ptr_t iter = node->children->first;
    while( iter ) {
      // destroy recursively
      vfs_destroy( ( vfs_node_ptr_t )iter->data );
      // get to next
      iter = iter->next;
    }
  }
  // finally destroy the node
  vfs_destroy_node( node );
}

/**
 * @brief setup virtual file system
 *
 * @param current_pid
 * @return root node
 */
vfs_node_ptr_t vfs_setup( pid_t current_pid ) {
  // necessary variables
  vfs_node_ptr_t root, ipc;
  // allocate minimum necessary nodes
  if (
    ! ( root = vfs_prepare_node( current_pid, "/", VFS_DIRECTORY, NULL ) )
    || ! ( ipc = vfs_prepare_node( current_pid, "ipc", VFS_DIRECTORY, root ) )
    || ! vfs_prepare_node( current_pid, "dev", VFS_DIRECTORY, root )
    || ! vfs_prepare_node( current_pid, "shared", VFS_DIRECTORY, ipc )
    || ! vfs_prepare_node( current_pid, "message", VFS_DIRECTORY, ipc )
  ) {
    // destroy recursively by starting with root
    vfs_destroy( root );
  }
  // return new vfs
  return root;
}

/**
 * @brief Add path to vfs
 *
 * @param root root object
 * @param handler handling process
 * @param path path to add
 * @param file boolean for file or false in case of folder
 * @return
 */
bool vfs_add_path(
  vfs_node_ptr_t root,
  pid_t handler,
  const char* set_path,
  bool file
) {
  // create folders if necessary
  __maybe_unused vfs_node_ptr_t start = NULL;
  // prepare path by trimming trailing separators
  char* path = malloc( sizeof( char ) * ( strlen( set_path ) + 1 ) );
  if ( ! path ) {
    return false;
  }
  // copy and trim trailing spaces
  path = strcpy( path, set_path );
  path = rtrim( ( char* )path, '/' );
  char* pos = NULL;
  char* last = strrchr( path, '/' );
  char tmp[ 256 ];
  //size_t idx = 0;
  size_t offset = 0;

  // create path
  do {
    pos = strchr( pos ? pos + 1 : path, '/' );

    // start at root
    if ( ! start ) {
      start = root;
      offset = 1;
      continue;
    }
    // get path part
    size_t len = ( size_t )( pos - path ) - offset;
    strncpy( tmp, path + offset, len );
    tmp[ len ] = '\0';
    offset += ( size_t )( pos - path );

    // add path or use exissting one
    vfs_node_ptr_t tmp_node = vfs_node_by_path_part( tmp, start );
    if ( tmp_node ) {
      start = tmp_node;
    } else {
      start = vfs_prepare_node( handler, tmp, VFS_DIRECTORY, start );
    }

    // handle add end point
    if ( pos == last ) {
      break;
    }
  } while( pos );

  // add end point
  size_t len = ( size_t )( last - path );
  strncpy( tmp, last + 1, len );
  if ( ! strlen( tmp ) ) {
    free( path );
    return true;
  }
  free( path );
  vfs_node_ptr_t tmp_node = vfs_node_by_path_part( tmp, start );
  if (
    tmp_node
    || ! vfs_prepare_node(
      handler,
      tmp,
      file ? VFS_FILE : VFS_DIRECTORY,
      start
    )
  ) {
    return false;
  }
  // return success
  return true;
}

/**
 * @brief Method to dump complete vfs
 *
 * @param root
 * @param level
 *
 * @todo replace tree view by simple list with complete paths
 */
void vfs_dump( vfs_node_ptr_t root, const char* level_prefix ) {
  size_t prefix_len = 0;
  if ( ! level_prefix ) {
    prefix_len = 2;
  } else {
    printf( "%s", level_prefix );
    if ( strlen( level_prefix ) != strlen( "/" ) ) {
      printf( "%c", '/' );
    }
    printf( "%s\r\n", root->name );
    prefix_len = strlen( level_prefix ) + 1;
  }

  // handle non folder
  if ( root->flags != VFS_DIRECTORY ) {
    return;
  }

  // iterate children
  list_item_ptr_t node = root->children->first;
  char* inner_prefix = malloc( sizeof( char ) * ( prefix_len + strlen( root->name ) + 1 ) );
  if ( inner_prefix ) {
    if ( NULL == level_prefix ) {
      strcat( inner_prefix, root->name );
    } else {
      strcpy( inner_prefix, level_prefix );
      if ( strlen( inner_prefix ) != strlen( "/" ) ) {
        strcat( inner_prefix, "/" );
      }
      strcat( inner_prefix, root->name );
    }

    while ( node ) {
      vfs_dump( ( vfs_node_ptr_t )node->data, inner_prefix );
      node = node->next;
    }
    free( inner_prefix );
  }
}
