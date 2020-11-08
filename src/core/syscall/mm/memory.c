
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

#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/task/process.h>
#include <core/syscall.h>
#include <core/interrupt.h>

/**
 * @brief Acquire memory
 *
 * @param context
 */
void syscall_memory_acquire( void* context ) {
  // get amount and current context
  uint32_t amount = syscall_get_parameter( context, 0 );
  uint32_t flag = syscall_get_parameter( context, 1 );
  virt_context_ptr_t virtual_context = task_thread_current_thread
    ->process
    ->virtual_context;

  // check for correct flag
  if ( flag > VIRT_PAGE_TYPE_NON_EXECUTABLE ) {
    syscall_populate_single_return( context, 0 );
    return;
  }

  // get full page count
  ROUND_UP_TO_FULL_PAGE( amount )

  // find free page range
  uintptr_t start = virt_find_free_page_range(
    virtual_context,
    amount
  );

  // map address range random
  if ( ! virt_map_address_range_random(
    virtual_context,
    start,
    amount,
    VIRT_MEMORY_TYPE_NORMAL,
    flag
  ) ) {
    syscall_populate_single_return( context, 0 );
    return;
  }

  // populate return
  syscall_populate_single_return( context, start );
}

/**
 * @brief Release memory
 *
 * @param context
 */
void syscall_memory_release( void* context ) {
  uintptr_t address = syscall_get_parameter( context, 0 );
  uintptr_t amount = syscall_get_parameter( context, 1 ) * PAGE_SIZE;
  virt_context_ptr_t virtual_context = task_thread_current_thread
      ->process
      ->virtual_context;

  // check if range is mapped in context
  if ( ! virt_is_mapped_in_context_range( virtual_context, address, amount ) ) {
    syscall_populate_single_return( context, false );
    return;
  }

  // return unmap result
  syscall_populate_single_return(
    context,
    virt_unmap_address_range( virtual_context, address, amount, true ) );
}
