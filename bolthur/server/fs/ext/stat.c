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

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include "stat.h"

/**
 * @fn int stat_cmp(struct stat_node*, struct stat_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
static int stat_cmp(
  struct stat_node* a,
  struct stat_node* b
) {
  return strcmp( a->path, b->path );
}

// define tree
STAT_TREE_DEFINE(
  stat_tree,
  stat_node,
  node,
  stat_cmp,
  __unused static inline
)
// create static tree
static struct stat_tree management_tree;

/**
 * @fn bool stat_node_setup(void)
 * @brief stat node setup
 *
 * @return
 */
bool stat_node_setup( void ) {
  stat_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn stat_node_t stat_node_extract*(const char*)
 * @brief Extract node
 *
 * @param name
 * @return
 */
stat_node_t* stat_node_extract( const char* path ) {
  // allocate node
  stat_node_t node;
  // clear out node
  memset( &node, 0, sizeof( node ) );
  // populate
  node.path = ( char* )path;
  // lookup
  stat_node_t* n = stat_node_tree_find( &management_tree, &node );
  // return node or null
  return n;
}

/**
 * @fn void stat_node_remove(const char*)
 * @brief Method to remove a node
 *
 * @param path
 */
void stat_node_remove( const char* path ) {
  stat_node_t* node = stat_node_extract( path );
  if ( ! node ) {
    return;
  }
  // remove node tree
  stat_node_tree_remove( &management_tree, node );
  // free node
  if ( node->path ) {
    free( node->path );
  }
  free( node );
}

/**
 * @fn bool stat_node_add(const char*, struct stat*)
 * @brief Helper to add a new node
 *
 * @param path
 * @param handler
 * @param st
 * @return
 */
bool stat_node_add( const char* path, struct stat* st ) {
  // allocate node
  stat_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate
  node->path = strdup( path );
  if ( ! node->path ) {
    free( node );
    return false;
  }
  node->st = malloc( sizeof( *st ) );
  if ( ! node->st ) {
    free( node->path );
    free( node );
    return false;
  }
  memcpy( node->st, st, sizeof( *st ) );
  // handle already existing and insert
  if (
    stat_node_tree_find( &management_tree, node )
    || stat_node_tree_insert( &management_tree, node )
  ) {
    free( node->path );
    free( node->st );
    free( node );
    return false;
  }
  return true;
}

/**
 * @fn struct stat stat_fetch*(const char*)
 * @brief Fetch cached stat information
 *
 * @param path
 * @return
 */
struct stat* stat_fetch( const char* path ) {
  stat_node_t* node = stat_node_extract( path );
  if ( ! node ) {
    return NULL;
  }
  return node->st;
}

/**
 * @fn bool stat_push(const char*, struct stat*)
 * @brief push stat information to cache
 *
 * @param path
 * @param st
 * @return
 */
bool stat_push( const char* path, struct stat* st ) {
  return stat_node_add( path, st );
}
