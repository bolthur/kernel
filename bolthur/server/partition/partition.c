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

#include <errno.h>
#include <libgen.h>
#include "partition.h"

// define tree
PARTITION_TREE_DEFINE(
  partition_tree,
  partition_node,
  node,
  partition_cmp,
  __unused static inline
)

/**
 * @fn int partition_cmp(struct partition_node*, struct partition_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
int partition_cmp(
  struct partition_node* a,
  struct partition_node* b
) {
  return strcmp( a->name, b->name );
}

// create static tree
static struct partition_tree management_tree;

/**
 * @fn bool partition_setup(void)
 * @brief mountpoint node setup
 *
 * @return
 */
bool partition_setup( void ) {
  partition_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn partition_node_t partition_extract*(const char*,bool)
 * @brief Extract watch information by name
 *
 * @param name
 * @param create
 * @return
 */
partition_node_t* partition_extract( const char* path, bool create ) {
  // allocate node
  partition_node_t* node = malloc( sizeof( *node ) );
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
  // lookup for node
  partition_node_t* found = partition_node_tree_find( &management_tree, node );
  // insert if not existing
  if ( ! found ) {
    if ( ! create ) {
      free( node->name );
      free( node );
      return NULL;
    }
    if ( partition_node_tree_insert( &management_tree, node ) ) {
      free( node->name );
      free( node );
      return NULL;
    }
    return partition_node_tree_find( &management_tree, node );
  }
  // free up again
  free( node->name );
  free( node );
  // return found
  return found;
}

/**
 * @fn int partition_add(const char*, mbr_table_entry_t*)
 * @brief Helper to add a watch node
 *
 * @param path
 * @param handler
 * @return
 */
int partition_add( const char* path, mbr_table_entry_t* mbr ) {
  // ensure that it doesn't exists
  partition_node_t* node = partition_extract( path, false );
  if ( node ) {
    return -EEXIST;
  }
  // try to create it
  node = partition_extract( path, true );
  if ( ! node ) {
    return -ENOMEM;
  }
  // allocate mbr if not allocated
  if ( ! node->data ) {
    // allocate node
    node->data = malloc( sizeof( *mbr ) );
    // handle error
    if ( ! node->data ) {
      return -ENOMEM;
    }
    // clear out data
    memset( node->data, 0, sizeof( *mbr ) );
    // copy data
    memcpy( node->data, mbr, sizeof( *mbr ) );
  }
  if ( ! node->device ) {
    // allocate space
    size_t device_length = sizeof( char ) * ( strlen( path ) + 1 );
    char* device = malloc( device_length );
    // handle errror
    if ( ! device ) {
      return -ENOMEM;
    }
    // clear space
    memset( device, 0, device_length );
    // loop through original and transfer everything except digit
    size_t resultIdx = 0;
    for ( size_t idx = 0; idx < device_length; idx++ ) {
      if ( ! isdigit( ( int )path[ idx ] ) ) {
        device[ resultIdx ] = path[ idx ];
        resultIdx++;
      }
    }
    EARLY_STARTUP_PRINT( "device = %s\r\n", device )
    // duplicate device
    node->device = device;
  }
  // return success
  return 0;
}

/**
 * @fn int partition_remove(const char*)
 * @brief Helper to add a watch node
 *
 * @param path
 * @param handler
 * @return
 */
int partition_remove( const char* path ) {
  partition_node_t* node = partition_extract( path, false );
  if ( ! node ) {
    return 0;
  }
  // remove node tree
  partition_node_tree_remove( &management_tree, node );
  // free up data
  free( node->name );
  free( node->data );
  free( node );
  // return success
  return 0;
}

/**
 * @fn void partition_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void partition_dump( void ) {
  EARLY_STARTUP_PRINT( "mountpoint node tree dump\r\n" )
  partition_tree_each(&management_tree, partition_node, n, {
      EARLY_STARTUP_PRINT("%s\r\n", n->name);
  });
}
