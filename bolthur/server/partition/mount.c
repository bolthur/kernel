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

#include <errno.h>
#include <libgen.h>
#include "mount.h"

// define tree
MOUNT_TREE_DEFINE(
  mount_tree,
  mount_node,
  node,
  mount_cmp,
  __unused static inline
)

/**
 * @fn int mount_cmp(struct mount_node*, struct mount_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
int mount_cmp( struct mount_node* a, struct mount_node* b ) {
  return strcmp( a->path, b->path );
}

// create static tree
static struct mount_tree management_tree;

/**
 * @fn bool mount_setup(void)
 * @brief mountpoint node setup
 *
 * @return
 */
bool mount_setup( void ) {
  mount_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn mount_node_t mount_extract*(const char*,bool)
 * @brief Extract mount information by name
 *
 * @param name
 * @param create
 * @return
 */
mount_node_t* mount_extract( const char* path, bool create ) {
  // allocate node
  mount_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return NULL;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->path = strdup( path );
  if ( ! node->path ) {
    free( node );
    return NULL;
  }
  // lookup for node
  mount_node_t* found = mount_node_tree_find( &management_tree, node );
  // insert if not existing
  if ( ! found ) {
    if ( ! create ) {
      free( node->path );
      free( node );
      return NULL;
    }
    if ( mount_node_tree_insert( &management_tree, node ) ) {
      free( node->path );
      free( node );
      return NULL;
    }
    return mount_node_tree_find( &management_tree, node );
  }
  // free up again
  free( node->path );
  free( node );
  // return found
  return found;
}

/**
 * @fn int mount_add(const char*, const char*, const char*, unsigned long, pid_t)
 * @brief Helper to add a mount node
 *
 * @param path
 * @param handler
 * @param type
 * @param flags
 * @param process
 * @return
 */
int mount_add(
  const char* path,
  const char* handler,
  const char* type,
  unsigned long flags,
  pid_t process
) {
  // ensure that it doesn't exists
  mount_node_t* node = mount_extract( path, false );
  if ( node ) {
    return -EEXIST;
  }
  // try to create it
  node = mount_extract( path, true );
  if ( ! node ) {
    return -ENOMEM;
  }
  // allocate mbr if not allocated
  if ( ! node->handler_name ) {
    node->handler_name = strdup( handler );
    if ( ! node->handler_name ) {
      return -ENOMEM;
    }
  }
  if ( ! node->type ) {
    node->type = strdup( type );
    if ( ! node->type ) {
      return -ENOMEM;
    }
  }
  if ( ! node->flags ) {
    node->flags = flags;
  }
  if ( ! node->process ) {
    node->process = process;
  }
  // return success
  return 0;
}

/**
 * @fn int mount_remove(const char*)
 * @brief Helper to add a watch node
 *
 * @param path
 * @param handler
 * @return
 */
int mount_remove( const char* path ) {
  mount_node_t* node = mount_extract( path, false );
  if ( ! node ) {
    return 0;
  }
  // remove node tree
  mount_node_tree_remove( &management_tree, node );
  // free up data
  free( node->path );
  free( node->handler_name );
  free( node );
  // return success
  return 0;
}

/**
 * @fn mount_node_t mount_extract_by_path_walk*(const char*)
 * @brief Method to get matching mount node by walking over the path
 *
 * @param path
 * @return
 */
mount_node_t* mount_extract_by_path_walk( const char* path ) {
  // allocate node
  mount_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->path = strdup( path );
  if ( ! node->path ) {
    free( node );
    return NULL;
  }

  // local duplicate for path
  char* p = strdup( path );
  if ( ! p ) {
    free( node->path );
    free( node );
    return NULL;
  }
  // set loop path and found
  char* loop_path = p;
  char* previous_loop = NULL;
  mount_node_t* found = NULL;
  // try to get mount point
  while ( ! found && *loop_path ) {
    // lookup
    found = mount_node_tree_find( &management_tree, node );
    // handle not found ( use dirname next )
    if ( ! found ) {
      if ( previous_loop ) {
        free( previous_loop );
      }
      previous_loop = strdup( loop_path );
      if ( ! previous_loop ) {
        break;
      }
      char* tmp = dirname( loop_path );
      // break loop
      if ( 0 == strcmp( previous_loop, tmp ) ) {
        break;
      }
      // overwrite loop_path
      loop_path = tmp;
      // clear out
      memset( node->path, 0, strlen( path ) );
      strcpy( node->path, loop_path );
    }
  }
  if ( previous_loop ) {
    free( previous_loop );
  }
  free( node->path );
  free( node );
  free( p );
  // handle found
  return found;
}

/**
 * @fn void mount_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void mount_dump( void ) {
  EARLY_STARTUP_PRINT( "mountpoint node tree dump\r\n" )
  mount_tree_each(&management_tree, mount_node, n, {
      EARLY_STARTUP_PRINT("%s\r\n", n->path);
  });
}
