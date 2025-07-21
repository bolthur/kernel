/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#include <string.h>
#include "response.h"

static response_message_entry_t response_error_message[] = {
  { "OK" },
  { "Memory error" },
  { "I/O error" },
  { "Mailbox error" },
  { "Not implemented" },
  { "Timeout while waiting for completion" },
  { "Unknown error" },
  { "Driver incompatible" },
};

/**
 * @fn const char* response_error(response_t)
 * @brief Method to translate response to error
 * @param num response to translate
 * @return translated error
 */
const char* response_error( response_t num ) {
  // static buffer
  static char buffer[ 1024 ];
  // set total length to length - 1 to leave space for 0 termination
  constexpr size_t total_length = sizeof( buffer ) - 1;
  // clear buffer
  memset( buffer, 0, sizeof( buffer ) );
  // determine entry count
  constexpr size_t error_count = sizeof( response_error_message )
    / sizeof( response_message_entry_t );
  // handle invalid error code
  if ( num >= error_count ) {
    return nullptr;
  }
  // push message string to buffer
  strncpy( buffer, response_error_message[ num ].message, total_length );
  // return buffer
  return buffer;
}
