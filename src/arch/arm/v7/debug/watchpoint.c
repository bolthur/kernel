
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

#include <stddef.h>
#include <core/debug/gdb.h>
#include <core/debug/watchpoint.h>

/**
 * @brief Method deactivates all watchpoints
 *
 * @todo add support for hardware watchpoints
 * @todo add support for software watchpoints
 */
void debug_watchpoint_disable( void ) {
  // skip if not initialized
  if (
    NULL == debug_watchpoint_manager
    || debug_gdb_get_running_flag()
  ) {
    return;
  }
}

/**
 * @brief Method activates all enabled watchpoint
 *
 * @todo add support for hardware watchpoint
 * @todo add support for software watchpoint
 */
void debug_watchpoint_enable( void ) {
  // skip if not initialized
  if (
    NULL == debug_watchpoint_manager
    || debug_gdb_get_running_flag()
  ) {
    return;
  }
}

/**
 * @brief Removes a watch point at address
 *
 * @param address address
 * @param remove remove completely
 *
 * @todo add logic for hardware watchpoints
 * @todo add logic for software watchpoints
 */
void debug_watchpoint_remove(
  __unused uintptr_t address,
  __unused bool remove
) {
}

/**
 * @brief Adds a watchpoint
 *
 * @param address address
 * @param enable watchpoint enabled
 *
 * @todo add logic for hardware watchpoints
 * @todo add logic for software watchpoints
 */
void debug_watchpoint_add(
  __unused uintptr_t address,
  __unused bool enable
) {
}
