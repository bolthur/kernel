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

#if ! defined( __CORE_DEBUG_BREAKPOINT__ )
#define __CORE_DEBUG_BREAKPOINT__

#include <stdint.h>
#include <stdbool.h>
#include <collection/list.h>

struct debug_breakpoint_entry {
  uintptr_t address;
  uintptr_t instruction;
  bool step;
  bool enabled;
};
typedef struct debug_breakpoint_entry debug_breakpoint_entry_t;
typedef struct debug_breakpoint_entry *debug_breakpoint_entry_ptr_t;

extern list_manager_ptr_t debug_breakpoint_manager;

bool debug_breakpoint_remove( uintptr_t, bool );
bool debug_breakpoint_remove_step( void );
bool debug_breakpoint_add( uintptr_t, bool, bool );
debug_breakpoint_entry_ptr_t debug_breakpoint_find( uintptr_t );
void debug_breakpoint_enable( void );
void debug_breakpoint_disable( void );
bool debug_breakpoint_init( void );
uintptr_t debug_breakpoint_get_instruction( void );

#endif
