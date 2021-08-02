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

#include <stddef.h>

#include <assert.h>
#if defined( PRINT_MM_VIRT )
  #include <debug/debug.h>
#endif
#include <entry.h>
#include <platform/rpi/peripheral.h>
#include <platform/rpi/mailbox/property.h>
#include <mm/phys.h>
#include <mm/virt.h>

#define GPIO_PERIPHERAL_BASE 0xF2000000
#if defined( BCM2836 ) || defined( BCM2837 )
  #define CPU_PERIPHERAL_BASE 0xF3000000
#endif
#define MAILBOX_PROPERTY_AREA 0xF3040000
#
/**
 * @brief Method to setup short descriptor paging
 */
void __bootstrap virt_startup_platform_setup( void ) {
  // cpu local peripherals
  #if defined( BCM2836 ) || defined( BCM2837 )
    uintptr_t cpu_peripheral_base = 0x40000000;
    size_t cpu_peripheral_size = 0x3FFFF;
    uintptr_t cpu_peripheral_end = cpu_peripheral_base + cpu_peripheral_size;

    while ( cpu_peripheral_base < cpu_peripheral_end ) {
      // identity map gpio
      virt_startup_map( ( uint64_t )cpu_peripheral_base, cpu_peripheral_base );
      // next page
      cpu_peripheral_base += PAGE_SIZE;
    }
  #endif

  // GPIO related
  #if defined( BCM2836 ) || defined( BCM2837 )
    uintptr_t gpio_peripheral_base = 0x3F000000;
    size_t gpio_peripheral_size = 0xFFFFFF;
  #else
    uintptr_t gpio_peripheral_base = 0x20000000;
    size_t gpio_peripheral_size = 0xFFFFFF;
  #endif
  uintptr_t gpio_peripheral_end = gpio_peripheral_base + gpio_peripheral_size;

  // map gpio if set
  while ( gpio_peripheral_base < gpio_peripheral_end ) {
    // identity map gpio
    virt_startup_map( ( uint64_t )gpio_peripheral_base, gpio_peripheral_base );
    // next page
    gpio_peripheral_base += PAGE_SIZE;
  }
}

/**
 * @brief Initialize virtual memory management
 */
void virt_platform_init( void ) {
  uintptr_t start;
  uintptr_t virtual;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "Map peripherals %p - %p\r\n",
      ( void* )peripheral_base_get( PERIPHERAL_GPIO ),
      ( void* )peripheral_end_get( PERIPHERAL_GPIO )
    )
  #endif

  // set start and virtual
  start = peripheral_base_get( PERIPHERAL_GPIO );
  virtual = GPIO_PERIPHERAL_BASE;

  // map peripherals
  while ( start < peripheral_end_get( PERIPHERAL_GPIO ) ) {
    // map
    assert( virt_map_address(
      virt_current_kernel_context,
      virtual,
      start,
      VIRT_MEMORY_TYPE_DEVICE,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) )

    // increase start and virtual
    start += PAGE_SIZE;
    virtual += PAGE_SIZE;
  }
  // handle local peripherals
  #if defined( BCM2836 ) || defined( BCM2837 )
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "Map local peripherals %p - %p\r\n",
        ( void* )peripheral_base_get( PERIPHERAL_LOCAL ),
        ( void* )peripheral_end_get( PERIPHERAL_LOCAL ) )
    #endif

    // set start and virtual
    start = peripheral_base_get( PERIPHERAL_LOCAL );
    virtual = CPU_PERIPHERAL_BASE;
    // map peripherals
    while ( start < peripheral_end_get( PERIPHERAL_LOCAL ) ) {
      // map
      assert( virt_map_address(
        virt_current_kernel_context,
        virtual,
        start,
        VIRT_MEMORY_TYPE_DEVICE,
        VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
      ) )
      // increase start and virtual
      start += PAGE_SIZE;
      virtual += PAGE_SIZE;
    }
  #endif

  // map mailbox buffer
  assert( virt_map_address(
    virt_current_kernel_context,
    MAILBOX_PROPERTY_AREA,
    ( uintptr_t )ptb_buffer_phys,
    VIRT_MEMORY_TYPE_DEVICE,
    VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
  ) )
}

/**
 * @brief Platform post initialization routine
 */
void virt_platform_post_init( void ) {
  // set new peripheral base
  peripheral_base_set( GPIO_PERIPHERAL_BASE, PERIPHERAL_GPIO );
  // Adjust base address of cpu peripheral
  peripheral_base_set( CPU_PERIPHERAL_BASE, PERIPHERAL_LOCAL );
  // set mailbox property pointer
  ptb_buffer = ( int32_t* )MAILBOX_PROPERTY_AREA;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "Set new gpio peripheral base to %p\r\n",
      ( void* )GPIO_PERIPHERAL_BASE )
    DEBUG_OUTPUT( "Set new cpu peripheral base to %p\r\n",
      ( void* )CPU_PERIPHERAL_BASE )
    DEBUG_OUTPUT( "Set mailbox property buffer to %p\r\n",
      ( void* )MAILBOX_PROPERTY_AREA )
  #endif
}
