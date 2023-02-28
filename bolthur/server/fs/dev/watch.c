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
#include "watch.h"

// define tree
WATCH_TREE_DEFINE(
  watch_tree,
  watch_node,
  node,
  watch_cmp,
  __unused static inline
)
WATCH_TREE_DEFINE(
  watch_pid_tree,
  watch_pid,
  node,
  watch_pid_cmp,
  __unused
)

/**
 * @fn int watch_cmp(struct watch_node*, struct watch_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
int watch_cmp(
  struct watch_node* a,
  struct watch_node* b
) {
  return strcmp( a->name, b->name );
}

/**
 * @fn int watch_pid_cmp(struct watch_node*, struct watch_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
int watch_pid_cmp(
  struct watch_pid* a,
  struct watch_pid* b
) {
  return a->process == b->process;
}

/**
 * @fn void watch_pid_destroy(struct watch_pid*)
 * @brief Helper to destroy watch pid node
 *
 * @param node
 */
void watch_pid_destroy( struct watch_pid* node ) {
  free( node );
}

// create static tree
static struct watch_tree management_tree;

/**
 * @fn int watch_pid_setup(watch_node_t*)
 * @brief Helper to prepare watch pid subtree
 *
 * @param watch
 * @return
 */
static int watch_pid_setup( watch_node_t* watch ) {
  if ( watch->pid ) {
    return 0;
  }
  // allocate some space
  watch->pid = malloc( sizeof( struct watch_pid_tree ) );
  if ( ! watch->pid ) {
    return -ENOMEM;
  }
  // clear out
  memset( watch->pid, 0, sizeof( struct watch_pid_tree ) );
  // setup tree
  watch_pid_tree_init( watch->pid );
  // return success
  return 0;
}

/**
 * @fn bool watch_setup(void)
 * @brief mountpoint node setup
 *
 * @return
 */
bool watch_setup( void ) {
  watch_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn watch_node_t watch_extract*(const char*,bool)
 * @brief Extract watch information by name
 *
 * @param name
 * @param create
 * @return
 */
watch_node_t* watch_extract( const char* path, bool create ) {
  // allocate node
  watch_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return NULL;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate name
  node->name = strdup( path );
  if ( ! node->name ) {
    free( node );
    return NULL;
  }
  // populate tree
  if ( 0 != watch_pid_setup( node ) ) {
    free( node->name );
    free( node );
    return NULL;
  }
  // lookup for node
  watch_node_t* found = watch_node_tree_find( &management_tree, node );
  // insert if not existing
  if ( ! found ) {
    if ( ! create ) {
      watch_pid_tree_destroy( node->pid, watch_pid_destroy );
      free( node->name );
      free( node );
      return NULL;
    }
    if ( watch_node_tree_insert( &management_tree, node ) ) {
      watch_pid_tree_destroy( node->pid, watch_pid_destroy );
      free( node->name );
      free( node );
      return NULL;
    }
    return watch_node_tree_find( &management_tree, node );
  }
  // free up again
  free( node->name );
  free( node );
  // return found
  return found;
}

/**
 * @fn int watch_add(const char*, pid_t, struct stat*)
 * @brief Helper to add a watch node
 *
 * @param path
 * @param handler
 * @param st
 * @return
 */
int watch_add( const char* path, pid_t handler ) {
  watch_node_t* node = watch_extract( path, true );
  if ( ! node ) {
    return -ENOMEM;
  }
  // allocate sub node
  watch_pid_t* pid_node = malloc( sizeof( *pid_node ) );
  // handle error
  if ( ! pid_node ) {
    return -ENOMEM;
  }
  // clear out node
  memset( pid_node, 0, sizeof( *pid_node ) );
  // fill information
  pid_node->process = handler;
  // handle already existing and insert
  if (
    watch_pid_tree_find( node->pid, pid_node )
    || watch_pid_tree_insert( node->pid, pid_node )
  ) {
    free( pid_node );
    return -EEXIST;
  }
  return 0;
}

/**
 * @fn int watch_remove(const char*, pid_t)
 * @brief Helper to add a watch node
 *
 * @param path
 * @param handler
 * @return
 */
int watch_remove( const char* path, pid_t handler ) {
  watch_node_t* node = watch_extract( path, true );
  if ( ! node ) {
    return -ENOMEM;
  }
  // allocate sub node
  watch_pid_t* pid_node = malloc( sizeof( *pid_node ) );
  // handle error
  if ( ! pid_node ) {
    return -ENOMEM;
  }
  // clear out node
  memset( pid_node, 0, sizeof( *pid_node ) );
  // fill information
  pid_node->process = handler;
  // handle not existing
  watch_pid_t* lookup_found = watch_pid_tree_find( node->pid, pid_node );
  if ( ! lookup_found ) {
    free( pid_node );
    return 0;
  }
  // handle removal and cleanup
  watch_pid_tree_remove( node->pid, lookup_found );
  watch_pid_destroy( lookup_found );
  // cleanup temporary node
  watch_pid_destroy( pid_node );
  // return success
  return 0;
}

/**
 * @fn void watch_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void watch_dump( void ) {
  EARLY_STARTUP_PRINT( "mountpoint node tree dump\r\n" )
  watch_tree_each(&management_tree, watch_node, n, {
      EARLY_STARTUP_PRINT("%s\r\n", n->name);
  });
}
