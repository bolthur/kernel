
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

#include <stddef.h>

#include "kernel/irq.h"
#include "kernel/panic.h"

/**
 * @brief Register IRQ handler
 *
 * @param num IRQ/FIQ to bind
 * @param func Callback to bind
 * @param fast flag to bind FIQ
 */
void irq_register_handler( uint8_t num, irq_callback_t func, bool fast ) {
  // normal irq
  if ( ! fast ) {
    // validate irq number
    ASSERT( irq_validate_number( num ) );

    // check for empty
    ASSERT( ! irq_callback_map[ num ] );

    // map handler
    irq_callback_map[ num ] = func;

    return;
  }

  // validate irq number
  ASSERT( irq_validate_number( num ) );

  // check for empty
  ASSERT( ! fast_irq_callback_map[ num ] );

  // map handler
  fast_irq_callback_map[ num ] = func;
}

/**
 * @brief Get IRQ/FIQ by number
 *
 * @param num IRQ/FIQ number
 * @param fast Flag to determine FIQ
 * @return irq_callback_t Bound callback or NULL
 */
irq_callback_t irq_get_handler( uint8_t num, bool fast ) {
  // normal irq
  if ( ! fast ) {
    // validate irq number
    ASSERT( irq_validate_number( num ) );

    // return handler
    return ! irq_callback_map[ num ]
      ? NULL
      : irq_callback_map[ num ];
  }

  // validate irq number
  ASSERT( irq_validate_number( num ) );

  // return handler
  return ! fast_irq_callback_map[ num ]
    ? NULL
    : fast_irq_callback_map[ num ];
}
