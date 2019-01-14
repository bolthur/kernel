
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "kernel/irq.h"
#include "kernel/event.h"
#include "kernel/panic.h"

/**
 * @brief Event list containing valid events
 */
static const char* event_list[] = { "timer", "serial" };

/**
 * @brief Event callback map
 */
static event_callback_map_t map[ MAX_BOUND_EVENT ];

/**
 * @brief Last bound entry
 */
static uint32_t last_entry = 0;

/**
 * @brief Initialize events
 */
void event_init() {
  // initialize map with 0
  memset( ( void* )&map, 0, sizeof( event_callback_map_t ) * MAX_BOUND_EVENT );

  // init irq events
  irq_setup_event();
}

/**
 * @brief Bind event handler
 *
 * @param type Event handler type
 * @param callback Callback to bind
 *
 * @todo Make variable amount of handlers possible
 */
void event_bind_handler( event_type_t type, event_callback_t callback ) {
  // check for not to much event and valid event name
  ASSERT( last_entry < MAX_BOUND_EVENT );

  // copy name with restriction and set callback
  strncpy( ( char* )&map[ last_entry ].event, event_list[ type ], MAX_EVENT_NAME );
  map[ last_entry ].map[ 0 ] = callback;

  // increase last entry
  last_entry += 1;
}

/**
 * @brief Unbind event handler
 *
 * @param type Event handler type
 * @param callback Callback to bind
 *
 * @todo Add function logic
 */
void event_unbind_handler( event_type_t type, event_callback_t callback ) {
  ( void )type;
  ( void )callback;
  PANIC( "event unbind not yet supported" );
}
