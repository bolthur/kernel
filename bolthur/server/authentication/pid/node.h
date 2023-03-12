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

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include <sys/tree.h>

#ifndef _PID_NODE_H
#define _PID_NODE_H

#define PID_TREE_DEFINE( name, type, field, cmp, attr ) \
  SPLAY_HEAD( name, type ); \
  SPLAY_PROTOTYPE( name, type, field, cmp ) \
  SPLAY_GENERATE( name, type, field, cmp ) \
  attr void type##_tree_init( struct name* t ) { \
    SPLAY_INIT( t ); \
  } \
  attr int type##_tree_empty( struct name* t ) { \
    return SPLAY_EMPTY( t ); \
  } \
  attr struct type* type##_tree_insert( struct name* t, struct type* e ) { \
    return SPLAY_INSERT( name, t, e ); \
  } \
  attr struct type* type##_tree_remove( struct name* t, struct type* e ) { \
    return SPLAY_REMOVE( name, t, e ); \
  } \
  attr struct type* type##_tree_find( struct name* t, struct type* e ) { \
    return SPLAY_FIND( name, t, e ); \
  } \
  attr struct type* type##_tree_min( struct name* t ) { \
    return SPLAY_MIN( name, t ); \
  } \
  attr struct type* type##_tree_max( struct name* t ) { \
    return SPLAY_MAX( name, t ); \
  } \
  attr struct type* type##_tree_next( struct name* t, struct type* e ) { \
    return SPLAY_NEXT( name, t, e ); \
  } \
  attr void type##_tree_apply( struct name* t, void( *cb )( struct type* ) ) { \
    pid_node_tree_each( t, type, e, cb( e ) ); \
  } \
  attr void type##_tree_destroy( struct name* t, void( *free_cb )( struct type* ) ) { \
    pid_node_tree_each_safe( t, type, e, free_cb( type##_tree_remove( t, e ) ) ); \
  }

#define pid_node_tree_each( t, type, e, block ) { \
    struct type* e; \
    for ( e = type##_tree_min( t ); e; e = type##_tree_next( t, e ) ) { \
      block; \
    }\
  }

#define pid_node_tree_each_safe( t, type, e, block ) { \
    struct type* e; \
    struct type* __tmp; \
    for ( \
      e = type##_tree_min( t ); \
      e && ( __tmp = type##_tree_next( t, e ), e ); \
      e = __tmp \
    ) { \
      block; \
    }\
  }

typedef struct pid_node {
  pid_t pid;
  uid_t uid;
  SPLAY_ENTRY( pid_node ) node;
  //gid_t gid[];
} pid_node_t;

bool pid_node_setup( void );
pid_node_t* pid_node_extract( pid_t );
bool pid_node_add( pid_t, uid_t );
void pid_node_remove( pid_t );
void pid_node_dump( void );

#endif
