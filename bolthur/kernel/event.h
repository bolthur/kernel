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

#if ! defined( _EVENT_H )
#define _EVENT_H

#include <stdbool.h>
#include <collection/list.h>
#include <collection/avl.h>
#include <stack.h>

#define EVENT_DETERMINE_ORIGIN( o ) \
  ( ! o || ! stack_is_kernel( ( uintptr_t )o ) ) \
    ? EVENT_ORIGIN_USER : EVENT_ORIGIN_KERNEL

typedef enum {
  EVENT_PROCESS = 1,
  EVENT_SERIAL,
  EVENT_DEBUG,
  EVENT_INTERRUPT_CLEANUP
} event_type_t;

typedef enum {
  EVENT_ORIGIN_KERNEL = 1,
  EVENT_ORIGIN_USER,
} event_origin_t;

struct event_manager {
  avl_tree_ptr_t tree;
  list_manager_ptr_t queue_kernel;
  list_manager_ptr_t queue_user;
};

struct event_block {
  avl_node_t node;
  event_type_t type;
  list_manager_ptr_t handler;
  list_manager_ptr_t post;
};

typedef void ( *event_callback_t )( event_origin_t, void* data );

struct callback {
  event_callback_t callback;
};

typedef struct callback event_callback_wrapper_t;
typedef struct callback *event_callback_wrapper_ptr_t;
typedef struct event_manager event_manager_t;
typedef struct event_manager *event_manager_ptr_t;
typedef struct event_block event_block_t;
typedef struct event_block *event_block_ptr_t;

#define EVENT_GET_BLOCK( n ) \
  ( event_block_ptr_t )( ( uint8_t* )n - offsetof( event_block_t, node ) )

bool event_init_get( void );
bool event_init( void );
bool event_bind( event_type_t, event_callback_t, bool );
void event_unbind( event_type_t, event_callback_t, bool );
void event_handle( void* );
bool event_enqueue( event_type_t, event_origin_t );

#endif
