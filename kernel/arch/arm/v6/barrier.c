
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <arch/arm/barrier.h>

/**
 * @brief Data memory barrier invalidation
 */
void __attribute__(( optimize( "O0" ) )) barrier_data_mem( void ) {
  __asm__ __volatile__ ( "mcr p15, #0, %[zero], c7, c10, #5" : : [ zero ] "r" ( 0 ) );
}

/**
 * @brief Data sync barrier invalidation
 */
void __attribute__(( optimize( "O0" ) )) barrier_data_sync( void ) {
  __asm__ __volatile__ ( "mcr p15, #0, %[zero], c7, c10, #4" : : [ zero ] "r" ( 0 ) );
}
