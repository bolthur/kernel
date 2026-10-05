/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#ifndef _ARCH_ARM_BARRIER_H
#define _ARCH_ARM_BARRIER_H

/**
 * @fn void barrier_data_mem(void)
 * @brief Data memory barrier invalidation
 */
[[maybe_unused]] __no_stack_protector __attribute__((always_inline)) static inline void barrier_data_mem( void ) {
  #if defined( ARCH_ARM_V6 )
    __asm__ __volatile__ ( "mcr p15, #0, %[zero], c7, c10, #5" : : [ zero ] "r" ( 0 ) : "memory" );
  #elif defined( ARCH_ARM_V7 )
    __asm__( "dmb" ::: "memory" );
  #elif defined( ARCH_ARM_v8 )
  #endif
}

/**
 * @fn void barrier_data_sync(void)
 * @brief Data sync barrier invalidation
 */
[[maybe_unused]] __no_stack_protector __attribute__((always_inline)) static inline void barrier_data_sync( void ) {
  #if defined( ARCH_ARM_V6 )
    __asm__ __volatile__ ( "mcr p15, #0, %[zero], c7, c10, #4" : : [ zero ] "r" ( 0 ) : "memory" );
  #elif defined( ARCH_ARM_V7 )
    __asm__( "dsb" ::: "memory" );
  #elif defined( ARCH_ARM_v8 )
  #endif
}

/**
 * @fn void barrier_instruction_sync(void)
 * @brief Instruction synchronization invalidation
 */
[[maybe_unused]] __no_stack_protector __attribute__((always_inline)) static inline void barrier_instruction_sync( void ) {
  #if defined( ARCH_ARM_V6 )
  __asm__ __volatile__ ( "mcr p15, #0, %[zero], c7, c5, #4" : : [ zero ] "r" ( 0 ) : "memory" );
  #elif defined( ARCH_ARM_V7 )
    __asm__( "isb" ::: "memory" );
  #elif defined( ARCH_ARM_v8 )
  #endif
}

#endif
