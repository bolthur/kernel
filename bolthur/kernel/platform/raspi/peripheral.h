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

#ifndef _PLATFORM_RASPI_PERIPHERAL_H
#define _PLATFORM_RASPI_PERIPHERAL_H

#include <stdint.h>
#include <stddef.h>

// initial setup of peripheral base
#if defined( BCM2709 ) || defined( BCM2710 )
  #define PERIPHERAL_GPIO_BASE 0x3F000000
  #define PERIPHERAL_GPIO_SIZE 0xFFFFFF
  #define PERIPHERAL_CPU_BASE 0x40000000
  #define PERIPHERAL_CPU_SIZE 0x3FFFF
#else
  #define PERIPHERAL_GPIO_BASE 0x20000000
  #define PERIPHERAL_GPIO_SIZE 0xFFFFFF
  #define PERIPHERAL_CPU_BASE 0
  #define PERIPHERAL_CPU_SIZE 0
#endif

typedef enum {
  PERIPHERAL_GPIO,
  PERIPHERAL_LOCAL,
} peripheral_type_t;

extern uintptr_t gpio_peripheral_base;
extern size_t gpio_peripheral_size;
extern uintptr_t cpu_peripheral_base;
extern size_t cpu_peripheral_size;

void peripheral_base_set( uintptr_t, peripheral_type_t );

/**
 * @fn uintptr_t peripheral_base_get(peripheral_type_t)
 * @brief Method to get peripheral base address
 *
 * @param type peripheral type
 * @return uintptr_t Peripheral base address
 */
[[maybe_unused]] __no_stack_protector __attribute__((always_inline)) static inline uintptr_t peripheral_base_get( const peripheral_type_t type ) {
  if ( PERIPHERAL_LOCAL == type ) {
    return cpu_peripheral_base;
  }
  if ( PERIPHERAL_GPIO == type ) {
    return gpio_peripheral_base;
  }
  return 0;
}

/**
 * @fn uintptr_t peripheral_end_get(peripheral_type_t)
 * @brief Method to get peripheral base address
 *
 * @param type peripheral type
 * @return uintptr_t Peripheral end address
 */
[[maybe_unused]] __no_stack_protector __attribute__((always_inline)) static inline uintptr_t peripheral_end_get( const peripheral_type_t type ) {
  if ( PERIPHERAL_LOCAL == type ) {
    return cpu_peripheral_base + cpu_peripheral_size;
  }
  if ( PERIPHERAL_GPIO == type ) {
    return gpio_peripheral_base + gpio_peripheral_size;
  }
  return 0;
}

#endif
