
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if ! defined( __KERNEL_IRQ__ )
#define __KERNEL_IRQ__

#include <avl.h>
#include <list.h>
#include <stdint.h>
#include <stdbool.h>

typedef void ( *irq_callback_t )( void** );

typedef struct {
  avl_tree_ptr_t normal_interrupt;
  avl_tree_ptr_t fast_interrupt;
} irq_manager_t, *irq_manager_ptr_t;

typedef struct {
  avl_node_t node;
  uint8_t interrupt;
  list_manager_ptr_t callback_list;
} irq_block_t, *irq_block_ptr_t;

typedef struct {
  irq_callback_t callback;
} irq_callback_wrapper_t, *irq_callback_wrapper_ptr_t;

#define IRQ_GET_BLOCK( n ) \
  ( irq_block_ptr_t )( ( uint8_t* )n - offsetof( irq_block_t, node ) )

int8_t irq_get_pending( bool );
void irq_disable( void );
void irq_enable( void );
bool irq_validate_number( uint8_t );
void irq_init( void );
void irq_handle( uint8_t, bool, void** );
void irq_register_handler( uint8_t, irq_callback_t, bool );
void irq_unregister_handler( uint8_t, irq_callback_t, bool );

#endif
