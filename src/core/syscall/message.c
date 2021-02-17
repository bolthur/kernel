
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

#include <core/syscall.h>
#if defined( PRINT_SYSCALL )
  #include <core/debug/debug.h>
#endif

/**
 * @brief Acquire message descriptor
 *
 * @param context
 */
void syscall_message_acquire( __unused void* context ) {
}

/**
 * @brief Release message descriptor
 *
 * @param context
 */
void syscall_message_release( __unused void* context ) {
}

/**
 * @brief Send message
 *
 * @param context
 */
void syscall_message_send( __unused void* context ) {
}

/**
 * @brief Receive message
 *
 * @param context
 */
void syscall_message_receive( __unused void* context ) {
}