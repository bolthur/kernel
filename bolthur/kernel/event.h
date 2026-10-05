/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#ifndef _EVENT_H
#define _EVENT_H

#include "../library/collection/list/list.h"
#include "../library/collection/avl/avl.h"

typedef enum {
  EVENT_PROCESS = 1U << 0,
  EVENT_PROCESS_CLEANUP = 1U << 1,
  EVENT_SERIAL = 1U << 2,
  EVENT_DEBUG = 1U << 3,
  EVENT_INTERRUPT_CLEANUP = 1U << 4,
} event_type_t;

typedef enum {
  EVENT_ORIGIN_KERNEL = 1,
  EVENT_ORIGIN_USER,
} event_origin_t;

struct event_manager {
  avl_tree_t* tree;
  uint32_t queue_bitmap;
};

struct event_block {
  avl_node_t node;
  event_type_t type;
  list_manager_t* handler;
  list_manager_t* post;
};

typedef void ( *event_callback_t )( event_origin_t, void* data );

struct callback {
  event_callback_t callback;
};

typedef struct callback event_callback_wrapper_t;
typedef struct event_manager event_manager_t;
typedef struct event_block event_block_t;

#define EVENT_GET_BLOCK( n ) \
  ( event_block_t* )( ( uint8_t* )n - offsetof( event_block_t, node ) )

bool event_init_get( void );
bool event_init( void );
bool event_bind( event_type_t, event_callback_t, bool );
void event_unbind( event_type_t, event_callback_t, bool );
void event_handle( void* );
void event_enqueue( event_type_t );
event_origin_t event_determine_origin( const void* );

#endif
