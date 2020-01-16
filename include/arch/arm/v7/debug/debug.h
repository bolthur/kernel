
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

#if ! defined( __ARCH_ARM_V7_DEBUG_DEBUG__ )
#define __ARCH_ARM_V7_DEBUG_DEBUG__

#include <stdbool.h>

bool debug_check_data_fault_status( void );
bool debug_check_instruction_fault( void );
bool debug_is_debug_exception( void );

void debug_enable_debug_monitor( void );
void debug_disable_debug_monitor( void );
bool debug_set_breakpoint( uintptr_t );
bool debug_remove_breakpoint( uintptr_t );
bool debug_enable_single_step( uintptr_t );
bool debug_disable_single_step( void );

#endif
