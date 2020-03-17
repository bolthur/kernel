
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

#if ! defined( __CORE_DEBUG_WATCHPOINT__ )
#define __CORE_DEBUG_WATCHPOINT__

#include <stdint.h>
#include <stdbool.h>
#include <list.h>

typedef struct {
  uintptr_t address;
  bool enabled;
} debug_watchpoint_entry_t, *debug_watchpoint_entry_ptr_t;

extern list_manager_ptr_t debug_watchpoint_manager;

void debug_watchpoint_remove( uintptr_t, bool );
void debug_watchpoint_add( uintptr_t, bool );
debug_watchpoint_entry_ptr_t debug_watchpoint_find( uintptr_t );
void debug_watchpoint_enable( void );
void debug_watchpoint_disable( void );
void debug_watchpoint_init( void );

#endif
