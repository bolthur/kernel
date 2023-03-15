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

#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <sys/bolthur.h>
#include "node.h"

/**
 * @fn int pid_cmp(struct pid_node*, struct pid_node*)
 * @brief Comparison function for tree
 *
 * @param a
 * @param b
 * @return
 */
static int pid_cmp(
  struct pid_node* a,
  struct pid_node* b
) {
  if ( a->pid == b->pid ) {
    return 0;
  }
  return a->pid > b->pid ? 1 : -1;
}

// define tree
PID_TREE_DEFINE(
  pid_tree,
  pid_node,
  node,
  pid_cmp,
  __unused static inline
)
// create static tree
static struct pid_tree management_tree;

/**
 * @fn bool pid_node_setup(void)
 * @brief pid node setup
 *
 * @return
 */
bool pid_node_setup( void ) {
  pid_node_tree_init( &management_tree );
  return true;
}

/**
 * @fn pid_node_t pid_node_extract*(pid_t)
 * @brief Extract node
 *
 * @param name
 * @return
 */
pid_node_t* pid_node_extract( pid_t pid ) {
  // allocate node
  pid_node_t node;
  // clear out node
  memset( &node, 0, sizeof( node ) );
  // populate
  node.pid = pid;
  // lookup
  pid_node_t* n = pid_node_tree_find( &management_tree, &node );
  if ( ! n ) {
    return n;
  }
  if ( 0 >= n->group_count ) {
    // allocate group list
    size_t old_size = 0;
    gid_t* group_list = NULL;
    // open groups and passwd
    setgrent();
    setpwent();
    // loop through groups
    struct group* grp;
    while ( ( grp = getgrent() ) ) {
      // loop through group members
      for ( size_t idx = 0; grp->gr_mem[ idx ]; idx++ ) {
        // get entry by name
        struct passwd* pass = getpwnam( grp->gr_mem[ idx ] );
        if ( ! pass ) {
          free( group_list );
          return NULL;
        }
        // handle no match
        if ( pass->pw_uid != n->uid ) {
          continue;
        }
        // reallocate
        size_t new_size = old_size + 1;
        gid_t* temp;
        if ( group_list ) {
          temp = realloc( group_list, new_size * sizeof( gid_t ) );
        } else {
          temp = malloc( new_size * sizeof( gid_t ) );
        }
        if ( ! temp ) {
          free( group_list );
          return NULL;
        }
        group_list = temp;
        group_list[ old_size ] = grp->gr_gid;
        old_size = new_size;
      }
    }
    // close groups and passwd
    endpwent();
    endgrent();
    if ( group_list ) {
      // allocate new node
      pid_node_t* new_node = malloc(
        sizeof( *new_node ) + sizeof( gid_t ) * old_size );
      if ( ! new_node ) {
        free( group_list );
        return NULL;
      }
      memset( new_node, 0, sizeof( *new_node ) + sizeof( gid_t ) * old_size );
      // copy over stuff
      new_node->pid = n->pid;
      new_node->uid = n->uid;
      new_node->group_count = old_size;
      for ( size_t idx = 0; idx < old_size; idx++ ) {
        new_node->gid[ idx ] = group_list[ idx ];
      }
      // remove old node
      pid_node_tree_remove( &management_tree, n );
      // add new node
      if ( pid_node_tree_insert( &management_tree, new_node ) ) {
        free( new_node );
        free( group_list );
        return NULL;
      }
      // overwrite found node
      n = new_node;
    }
    // free again
    free( group_list );
  }
  // return node
  return n;
}

/**
 * @fn void pid_node_remove(pid_t)
 * @brief Method to remove a node
 *
 * @param path
 */
void pid_node_remove( pid_t pid ) {
  pid_node_t* node = pid_node_extract( pid );
  if ( ! node ) {
    return;
  }
  // remove node tree
  pid_node_tree_remove( &management_tree, node );
  // free node
  free( node );
}

/**
 * @fn bool pid_node_add(pid_t, uid_t)
 * @brief Helper to add a new node
 *
 * @param path
 * @param handler
 * @param st
 * @return
 */
bool pid_node_add( pid_t pid, uid_t user ) {
  // allocate node
  pid_node_t* node = malloc( sizeof( *node ) );
  // handle error
  if ( ! node ) {
    return false;
  }
  // clear out node
  memset( node, 0, sizeof( *node ) );
  // populate
  node->pid = pid;
  node->uid = user;
  node->group_count = 0;
  // handle already existing and insert
  if (
    pid_node_tree_find( &management_tree, node )
    || pid_node_tree_insert( &management_tree, node )
  ) {
    free( node );
    return false;
  }
  return true;
}

/**
 * @fn void pid_node_dump(void)
 * @brief Simple method to dump mount point nodes
 */
void pid_node_dump( void ) {
  EARLY_STARTUP_PRINT( "pid node tree dump\r\n" )
  pid_node_tree_each(&management_tree, pid_node, n, {
    EARLY_STARTUP_PRINT( "%d | %d\r\n", n->pid, n->uid )
  });
}
