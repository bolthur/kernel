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

#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/tree.h>

#ifndef _HANDLE_H
#define _HANDLE_H

SPLAY_HEAD( handle_tree, handle_node );
void handle_node_tree_init( struct handle_tree* t );
int handle_node_tree_empty( struct handle_tree* t );
struct handle_node* handle_node_tree_insert( struct handle_tree* t, struct handle_node* e );
struct handle_node* handle_node_tree_remove( struct handle_tree* t, struct handle_node* e );
struct handle_node* handle_node_tree_find( struct handle_tree* t, struct handle_node* e );
struct handle_node* handle_node_tree_min( struct handle_tree* t );
struct handle_node* handle_node_tree_max( struct handle_tree* t );
struct handle_node* handle_node_tree_next( struct handle_tree* t, struct handle_node* e );
void handle_node_tree_apply( struct handle_tree* t, void( *cb )( struct handle_node* ) );
void handle_node_tree_destroy( struct handle_tree* t, void( *free_cb )( struct handle_node* ) );

#define HANDLE_TREE_DEFINE( name, type, field, cmp, attr ) \
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
    handle_node_tree_each( t, type, e, cb( e ) ); \
  } \
  attr void type##_tree_destroy( struct name* t, void( *free_cb )( struct type* ) ) { \
    handle_node_tree_each_safe( t, type, e, free_cb( type##_tree_remove( t, e ) ) ); \
  }

#define handle_node_tree_each( t, type, e, block ) { \
    struct type* e; \
    for ( e = type##_tree_min( t ); e; e = type##_tree_next( t, e ) ) { \
      block; \
    }\
  }

#define handle_node_tree_each_safe( t, type, e, block ) { \
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

typedef struct handle_node {
  /** @brief generated handle */
  int handle;
  /** @brief open flags */
  int flags;
  /** @brief mode */
  int mode;
  /** @brief current position */
  off_t pos;
  /** @brief file path */
  char path[ PATH_MAX ];
  /** @brief process handling file */
  pid_t handler;
  /** @brief stat information */
  struct stat info;
  /** @brief additional data */
  void* data;
  /** @brief tree data */
  SPLAY_ENTRY( handle_node ) node;
} handle_node_t;

int handle_get( handle_node_t**, pid_t, int );
int handle_generate( handle_node_t**, pid_t, pid_t, void*, const char*, int, int );
int handle_set( handle_node_t**, int, pid_t, pid_t, void*, const char*, int, int );
int handle_destory( pid_t, int );
void handle_destory_all( pid_t );

#endif
