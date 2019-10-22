
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

#if ! defined( __KERNEL_EVENT__ )
#define __KERNEL_EVENT__

#include <stdbool.h>
#include <list.h>

typedef enum {
  EVENT_TIMER = 0,
  EVENT_DUMMY_LAST
} event_type_t;

typedef struct {
  list_manager_ptr_t* list;
} event_manager_t, *event_manager_ptr_t;

typedef void ( *event_callback_t )( void** data );

typedef struct {
  event_callback_t callback;
} event_callback_wrapper_t, *event_callback_wrapper_ptr_t;

bool event_initialized_get( void );
void event_init( void );

bool event_bind( event_type_t, event_callback_t );
void event_unbind( event_type_t, event_callback_t );
void event_fire( event_type_t, void** );

#endif
