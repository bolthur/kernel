
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#define MAX_BOUND_EVENT 25
#define MAX_EVENT_NAME 25
#define MAX_EVENT_CALLBACK 1

#ifndef __KERNEL_EVENT__
#define __KERNEL_EVENT__

#if defined( __cplusplus )
extern "C" {
#endif

typedef enum {
  EVENT_TIMER,
  EVENT_SERIAL
} event_type_t;

typedef void ( *event_callback_t )( void *reg );

typedef struct {
  char event[ MAX_EVENT_NAME ];
  event_callback_t map[ MAX_EVENT_CALLBACK ];
} event_callback_map_t;

void event_bind_handler( event_type_t, event_callback_t );
void event_init();
void event_unbind_handler( event_type_t, event_callback_t );

#if defined( __cplusplus )
}
#endif

#endif
