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

#ifndef _WATCH_H
#define _WATCH_H

#define WATCH_TREE_DEFINE( name, type, field, cmp, attr ) \
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
    watch_tree_each( t, type, e, cb( e ) ); \
  } \
  attr void type##_tree_destroy( struct name* t, void( *free_cb )( struct type* ) ) { \
    watch_tree_each_safe( t, type, e, free_cb( type##_tree_remove( t, e ) ) ); \
  }

#define watch_tree_each( t, type, e, block ) { \
    struct type* e; \
    for ( e = type##_tree_min( t ); e; e = type##_tree_next( t, e ) ) { \
      block; \
    }\
  }

#define watch_tree_each_safe( t, type, e, block ) { \
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

struct watch_pid_tree;
typedef struct watch_pid {
  pid_t process;
  SPLAY_ENTRY( watch_pid ) node;
} watch_pid_t;

typedef struct watch_node {
  char* name;
  struct watch_pid_tree* pid;
  SPLAY_ENTRY( watch_node ) node;
} watch_node_t;

// tree stuff
struct watch_pid* watch_pid_tree_min( struct watch_pid_tree* );
struct watch_pid* watch_pid_tree_next( struct watch_pid_tree*, struct watch_pid* );
void watch_pid_tree_init( struct watch_pid_tree* );
int watch_pid_tree_empty( struct watch_pid_tree* );
struct watch_pid* watch_pid_tree_insert( struct watch_pid_tree*, struct watch_pid* );
struct watch_pid* watch_pid_tree_remove( struct watch_pid_tree*, struct watch_pid* );
struct watch_pid* watch_pid_tree_find( struct watch_pid_tree*, struct watch_pid* );
struct watch_pid* watch_pid_tree_max( struct watch_pid_tree* );
void watch_pid_tree_apply( struct watch_pid_tree*, void(*)( struct watch_pid* ) );
void watch_pid_tree_destroy( struct watch_pid_tree*, void(*)( struct watch_pid* ) );
// comparison stuff for splay
int watch_cmp( struct watch_node*, struct watch_node* );
int watch_pid_cmp( struct watch_pid*, struct watch_pid* );
void watch_pid_destroy( struct watch_pid* );
// generic stuff
bool watch_setup( void );
watch_node_t* watch_extract( const char*, bool );
int watch_add( const char*, pid_t );
int watch_remove( const char*, pid_t );
void watch_dump( void );

#endif
