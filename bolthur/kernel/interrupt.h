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

#if ! defined( __CORE_INTERRUPT__ )
#define __CORE_INTERRUPT__

#include <assert.h>
#include <collection/avl.h>
#include <collection/list.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <task/thread.h>

#define INTERRUPT_NESTED_MAX 3
#define INTERRUPT_DETERMINE_CONTEXT( c ) \
  c = NULL != c ? c : TASK_THREAD_GET_CONTEXT; \
  assert( c != NULL );

typedef void ( *interrupt_callback_t )( void* );

typedef enum {
  INTERRUPT_NORMAL = 1,
  INTERRUPT_FAST,
  INTERRUPT_SOFTWARE
} interrupt_type_t;

typedef enum {
  INTERRUPT_TOGGLE_AUTO,
  INTERRUPT_TOGGLE_ON,
  INTERRUPT_TOGGLE_OFF
} interrupt_toggle_state_t;

struct interrupt_manager {
  avl_tree_ptr_t normal_interrupt;
  avl_tree_ptr_t fast_interrupt;
  avl_tree_ptr_t software_interrupt;
};

struct interrupt_block {
  avl_node_t node;
  size_t interrupt;
  list_manager_ptr_t handler;
  list_manager_ptr_t post;
};

struct interrupt_callback {
  interrupt_callback_t callback;
};

typedef struct interrupt_manager interrupt_manager_t;
typedef struct interrupt_manager *interrupt_manager_ptr_t;
typedef struct interrupt_block interrupt_block_t;
typedef struct interrupt_block *interrupt_block_ptr_t;
typedef struct interrupt_callback interrupt_callback_wrapper_t;
typedef struct interrupt_callback *interrupt_callback_wrapper_ptr_t;

#define INTERRUPT_GET_BLOCK( n ) \
  ( interrupt_block_ptr_t )( ( uint8_t* )n - offsetof( interrupt_block_t, node ) )

int8_t interrupt_get_pending( bool );
void interrupt_toggle( interrupt_toggle_state_t );
void interrupt_disable( void );
void interrupt_enable( void );
bool interrupt_validate_number( size_t );
void interrupt_init( void );
void interrupt_arch_init( void );
void interrupt_post_init( void );
void interrupt_handle( size_t, interrupt_type_t, void* );
bool interrupt_register_handler( size_t, interrupt_callback_t, interrupt_type_t, bool );
bool interrupt_unregister_handler( size_t, interrupt_callback_t, interrupt_type_t, bool );
void interrupt_handle_possible( void*, bool );

#endif
