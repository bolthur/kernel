
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#if ! defined( __CORE_EVENT__ )
#define __CORE_EVENT__

#include <stdbool.h>
#include <list.h>
#include <avl.h>
#include <core/stack.h>

#define EVENT_DETERMINE_ORIGIN( o ) \
  ( o == NULL || ! stack_is_kernel( ( uintptr_t )o ) ) \
    ? EVENT_ORIGIN_USER : EVENT_ORIGIN_KERNEL

typedef enum {
  EVENT_TIMER = 1,
  EVENT_SERIAL,
  EVENT_DEBUG,
  EVENT_INTERRUPT_CLEANUP
} event_type_t;

typedef enum {
  EVENT_ORIGIN_KERNEL = 1,
  EVENT_ORIGIN_USER,
} event_origin_t;

typedef struct {
  avl_tree_ptr_t tree;
  list_manager_ptr_t queue_kernel;
  list_manager_ptr_t queue_user;
} event_manager_t, *event_manager_ptr_t;

typedef struct {
  avl_node_t node;
  event_type_t type;
  list_manager_ptr_t handler;
  list_manager_ptr_t post;
} event_block_t, *event_block_ptr_t;

typedef void ( *event_callback_t )( event_origin_t, void* data );

typedef struct {
  event_callback_t callback;
} event_callback_wrapper_t, *event_callback_wrapper_ptr_t;

#define EVENT_GET_BLOCK( n ) \
  ( event_block_ptr_t )( ( uint8_t* )n - offsetof( event_block_t, node ) )

bool event_initialized_get( void );
void event_init( void );
bool event_bind( event_type_t, event_callback_t, bool );
void event_unbind( event_type_t, event_callback_t, bool );
void event_handle( void* );
void event_enqueue( event_type_t, event_origin_t );

#endif
