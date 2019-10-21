
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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <kernel/task/thread.h>
#include <arch/arm/v7/cpu.h>

/**
 * @brief Method to create thread structure
 *
 * @param entry entry point of the thread
 * @param type thread type
 * @return void* pointer to thread structure
 *
 * @todo populate reserved context
 * @todo set entry point within created cpu context
 * @todo consider thread type within context prepare
 */
void* task_create_thread(
  __unused uintptr_t entry,
  __unused task_thread_type_t type
) {
  // create instance of cpu structure
  cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )malloc(
    sizeof( cpu_register_context_t ) );
  // assert malloc return
  assert( NULL != cpu );
  // preset with 0
  memset( ( void* )cpu, 0, sizeof( cpu_register_context_t ) );

  // fill context

  // return created context
  return ( void* )cpu;
}
