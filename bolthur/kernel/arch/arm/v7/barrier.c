/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <arch/arm/barrier.h>

/**
 * @brief Data memory barrier invalidation
 */
void barrier_data_mem( void ) {
  __asm__( "dmb" ::: "memory" );
}

/**
 * @brief Data sync barrier invalidation
 */
void barrier_data_sync( void ) {
  __asm__( "dsb" ::: "memory" );
}

/**
 * @brief Instruction synchronization invalidation
 */
void barrier_instruction_sync( void ) {
  __asm__( "isb" ::: "memory" );
}
