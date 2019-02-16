
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

#include <stdint.h>

#include "kernel/arch/arm/mm/virt.h"
#include "kernel/kernel/entry.h"

/**
 * @brief Initial kernel context
 */
sd_context_total_t SECTION( ".data.boot" ) initial_kernel_context;

/**
 * @brief Method to prepare section during initial boot
 *
 * @todo Check behaviour with less than 1 GB physical memory
 */
void SECTION( ".text.boot" ) boot_vendor_prepare( void ) {
  uint32_t x, y, reg;

  // get paging support from mmfr0
  __asm__ __volatile__( "mrc p15, 0, %0, c0, c1, 4" : "=r" ( reg ) : : "cc" );

  // strip out everything not needed
  reg &= 0xF;

  if (
    ! (
      ID_MMFR0_VSMA_SUPPORT_V7_PAGING_REMAP_ACCESS == reg
      || ID_MMFR0_VSMA_SUPPORT_V7_PAGING_PXN == reg
      || ID_MMFR0_VSMA_SUPPORT_V7_PAGING_LPAE == reg
    )
  ) {
    return;
  }

  for ( x = 0; x < 4096; x++ ) {
    initial_kernel_context.list[ x ] = 0;
  }

  // map first gb
  for ( x = 0; x < ( MAX_PHYSICAL_MEMORY >> 20 ); x++ ) {
    // offset
    y = x + ( KERNEL_OFFSET >> 20 );

    initial_kernel_context.section[ x ].privileged_execute_never = 0;
    initial_kernel_context.section[ x ].type = 1;
    initial_kernel_context.section[ x ].bufferable = 0;
    initial_kernel_context.section[ x ].cacheable = 0;
    initial_kernel_context.section[ x ].execute_never = 1;
    initial_kernel_context.section[ x ].domain = 0;
    initial_kernel_context.section[ x ].imp = 0;
    initial_kernel_context.section[ x ].access_permision_0 = 0x3;
    initial_kernel_context.section[ x ].tex = 0;
    initial_kernel_context.section[ x ].access_permision_1 = 0;
    initial_kernel_context.section[ x ].shareable = 0;
    initial_kernel_context.section[ x ].not_global = 0;
    initial_kernel_context.section[ x ].sbz = 0;
    initial_kernel_context.section[ x ].non_secure = 0;
    initial_kernel_context.section[ x ].frame = ( ( x << 20 ) >> 20 ) & 0xFFF;

    if ( 0 < KERNEL_OFFSET ) {
      initial_kernel_context.section[ y ].privileged_execute_never = 0;
      initial_kernel_context.section[ y ].type = 1;
      initial_kernel_context.section[ y ].bufferable = 0;
      initial_kernel_context.section[ y ].cacheable = 0;
      initial_kernel_context.section[ y ].execute_never = 1;
      initial_kernel_context.section[ y ].domain = 0;
      initial_kernel_context.section[ y ].imp = 0;
      initial_kernel_context.section[ y ].access_permision_0 = 0x3;
      initial_kernel_context.section[ y ].tex = 0;
      initial_kernel_context.section[ y ].access_permision_1 = 0;
      initial_kernel_context.section[ y ].shareable = 0;
      initial_kernel_context.section[ y ].not_global = 0;
      initial_kernel_context.section[ y ].sbz = 0;
      initial_kernel_context.section[ y ].non_secure = 0;
      initial_kernel_context.section[ y ].frame = ( ( x << 20 ) >> 20 ) & 0xFFF;
    }
  }

  #if defined( PLATFORM_RPI2_B ) || defined( PLATFORM_RPI3_B )
    x = ( 0x40000000 >> 20 );
    initial_kernel_context.section[ x ].privileged_execute_never = 0;
    initial_kernel_context.section[ x ].type = 1;
    initial_kernel_context.section[ x ].bufferable = 0;
    initial_kernel_context.section[ x ].cacheable = 0;
    initial_kernel_context.section[ x ].execute_never = 1;
    initial_kernel_context.section[ x ].domain = 0;
    initial_kernel_context.section[ x ].imp = 0;
    initial_kernel_context.section[ x ].access_permision_0 = 0x3;
    initial_kernel_context.section[ x ].tex = 0;
    initial_kernel_context.section[ x ].access_permision_1 = 0;
    initial_kernel_context.section[ x ].shareable = 0;
    initial_kernel_context.section[ x ].not_global = 0;
    initial_kernel_context.section[ x ].sbz = 0;
    initial_kernel_context.section[ x ].non_secure = 0;
    initial_kernel_context.section[ x ].frame = ( 0x40000000 >> 20 ) & 0xFFF;
  #endif

  // FIXME: RPI2 specific, might need rework for RPI zero and RPI3
  // invalidate tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) : "memory" );
  // invalidate caches
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 1" : : "r" ( 0 ) : "memory" );
  // wait for finished
  __asm__ __volatile__( "dsb" ::: "memory" );


  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // disable cache and icache
  reg = ( uint32_t )( ( int32_t )reg & ~( 1 << 2 ) ); // unset cache
  reg = ( uint32_t )( ( int32_t )reg & ~( 1 << 12 ) ); // unset icache
  // write changes
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );

  // Get content from auxiliary control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );
  // set smp bit
  reg |= ( 1 << 6 );
  // write changes
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );

  // Copy page table address to cp15
  __asm__ __volatile__( "mcr p15, 0, %0, c2, c0, 0" : : "r" ( initial_kernel_context.list ) : "memory" );

  // Set the access control to all-supervisor
  __asm__ __volatile__( "mcr p15, 0, %0, c3, c0, 0" : : "r" ( ~0 ) );

  // Get content from control register
  __asm__ __volatile__( "mrc p15, 0, %0, c1, c0, 0" : "=r" ( reg ) : : "cc" );

  // enable mmu by setting bit 0
  reg |= 0x1;

  // push back value with mmu enabled bit set
  __asm__ __volatile__( "mcr p15, 0, %0, c1, c0, 0" : : "r" ( reg ) : "cc" );
}
