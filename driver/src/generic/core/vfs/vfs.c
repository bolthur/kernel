
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
#include <libgen.h>
#include <sys/bolthur.h>

#include "list.h"
#include "vfs.h"
#include "util.h"

static vfs_node_ptr_t root;

/**
 * @brief helper to destroy a node
 *
 * @param node
 */
static void vfs_destroy_node( vfs_node_ptr_t node ) {
  if ( ! node ) {
    return;
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
  // copy name, save current pid as handler and set flag to directory
  strcpy( node->name, name );
  node->pid = pid;
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
  vfs_node_ptr_t ipc;
  // allocate minimum necessary nodes
  if (
    ! ( root = vfs_prepare_node( current_pid, "/", VFS_DIRECTORY, NULL ) )
    || ! ( ipc = vfs_prepare_node( current_pid, "ipc", VFS_DIRECTORY, root ) )
    || ! vfs_prepare_node( current_pid, "dev", VFS_DIRECTORY, root )
    || ! vfs_prepare_node( current_pid, "process", VFS_DIRECTORY, root )
    || ! vfs_prepare_node( current_pid, "shared", VFS_DIRECTORY, ipc )
    || ! vfs_prepare_node( current_pid, "message", VFS_DIRECTORY, ipc )
  ) {
    // destroy recursively by starting with root
    vfs_destroy( root );
    return NULL;
  }
  // return new vfs
  return root;
}

/**
 * @brief Add path to vfs
 *
 * @param node node object
 * @param handler handling process
 * @param path path to add
 * @param type file type
 * @return
 *
 * @todo add support for symlinks and hardlinks
 */
bool vfs_add_path(
  vfs_node_ptr_t node,
  pid_t handler,
  const char* path,
  vfs_entry_type_t type
) {
  // variables
  uint32_t flags = 0;

  // handle no parent node
  if ( ! node ) {
    return false;
  }

  // determine type
  switch( type ) {
    case VFS_ENTRY_TYPE_FILE:
      flags |= VFS_FILE;
      break;
    case VFS_ENTRY_TYPE_DIRECTORY:
      flags |= VFS_DIRECTORY;
      break;
    default:
      return false;
  }

  // return prepared node
  return vfs_prepare_node( handler, path, flags, node );
}

/**
 * @brief Method to dump complete vfs
 *
 * @param node
 * @param level
 */
void vfs_dump( vfs_node_ptr_t node, const char* level_prefix ) {
  // handle no root
  if ( ! node ) {
    vfs_dump( root, level_prefix );
    return;
  }

  size_t prefix_len = 0;
  if ( ! level_prefix ) {
    prefix_len = 2;
  } else {
    printf( "%s", level_prefix );
    if ( strlen( level_prefix ) != strlen( "/" ) ) {
      printf( "%c", '/' );
    }
    printf( "%s\r\n", node->name );
    prefix_len = strlen( level_prefix ) + 1;
  }

  // handle non folder
  if ( node->flags != VFS_DIRECTORY ) {
    return;
  }

  // iterate children
  list_item_ptr_t children = node->children->first;
  // allocate new inner prefix
  char* inner_prefix = malloc( sizeof( char ) * ( prefix_len + strlen( node->name ) + 1 ) );
  // prepare it
  if ( inner_prefix ) {
    // reset allocated memory
    memset( inner_prefix, 0, sizeof( char ) * ( prefix_len + strlen( node->name ) + 1 ) );
    // push content
    if ( NULL == level_prefix ) {
      strcat( inner_prefix, node->name );
    } else {
      strcpy( inner_prefix, level_prefix );
      if ( strlen( inner_prefix ) != strlen( "/" ) ) {
        strcat( inner_prefix, "/" );
      }
      strcat( inner_prefix, node->name );
    }
    // iterate children and dump
    while ( children ) {
      vfs_dump( ( vfs_node_ptr_t )children->data, inner_prefix );
      children = children->next;
    }
    // free prefix again
    free( inner_prefix );
  }
}

/**
 * @brief Helper to get children node by name of given node
 *
 * @param node root node
 * @param path path to lookup
 * @return found node or NULL
 */
vfs_node_ptr_t vfs_node_by_name(
  vfs_node_ptr_t node,
  const char* path
) {
  // local pointer for path
  char* p = ( char* )path;
  // skip leading slash
  if ( *p == '/' ) {
    p++;
  }
  // try to find item out of list
  list_item_ptr_t current = node->children->first;
  while ( current ) {
    // get vfs node
    vfs_node_ptr_t n = ( vfs_node_ptr_t )( current->data );
    // check for match
    if ( 0 == strcmp( p, n->name ) ) {
      return n;
    }
    // continue with next child
    current = current->next;
  }
  // return nothing found
  return NULL;
}

/**
 * @brief Helper to get path node by name relative to global root
 *
 * @param path path to lookup
 * @return found node or NULL
 */
vfs_node_ptr_t vfs_node_by_path( const char* path ) {
  // local pointer for path
  char* p = ( char* )path;
  // skip leading slash
  if ( *p == '/' ) {
    p++;
  }

  // start with root
  vfs_node_ptr_t current = root;
  // handle empty ( return root )
  if ( 0 == strlen( p ) ) {
    return current;
  }

  // setup index and length
  size_t len = strlen( p );

  // begin and end of path
  char* begin = p;
  char* end = p;

  // loop until end
  for( size_t idx = 0; idx <= len; idx++ ) {
    // handle no separator yet
    if( *end != '/' && *end != '\0' ) {
      end++;
      continue;
    }

    // calculate size for path part
    size_t inner = ( size_t )( end - begin + 1 );
    // acquire space for new path
    char* inner_path = malloc( sizeof( char ) * inner );
    if ( ! inner_path ) {
      return NULL;
    }
    // clear and copy over content
    memset( inner_path, 0, sizeof( char ) * inner );
    strncpy( inner_path, begin, inner - 1 );
    inner_path[ inner - 1 ] = '\0';
    // try to get node by name
    current = vfs_node_by_name( current, inner_path );
    // free up memory again
    free( inner_path );
    // handle no node found
    if ( ! current ) {
      return NULL;
    }
    // reset begin and continue
    begin = ++end;
  }
  // return found node
  return current;
}
