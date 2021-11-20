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

#include <stddef.h>
#include <task/thread.h>

#if ! defined( _TIMER_H )
#define _TIMER_H

struct timer_callback {
  size_t id;
  size_t expire;
  task_thread_ptr_t thread;
  size_t rpc;
};
typedef struct timer_callback timer_callback_entry_t;
typedef struct timer_callback* timer_callback_entry_ptr_t;

void timer_init( void );
void timer_platform_init( void );
size_t timer_get_frequency( void );
size_t timer_get_interval( void );
size_t timer_get_tick( void );

size_t timer_generate_id( void );
timer_callback_entry_ptr_t timer_register_callback( task_thread_ptr_t, size_t, size_t );
bool timer_unregister_callback( size_t );
void timer_handle_callback( void );

#endif
