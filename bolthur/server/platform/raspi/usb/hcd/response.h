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

#ifndef _RESPONSE_H
#define _RESPONSE_H

typedef enum{
  HCD_RESPONSE_OK = 0,
  HCD_RESPONSE_ERROR_MEMORY,
  HCD_RESPONSE_ERROR_IO,
  HCD_RESPONSE_ERROR_MAILBOX,
  HCD_RESPONSE_ERROR_EINVAL,
  HCD_RESPONSE_ERROR_NOT_IMPLEMENTED,
  HCD_RESPONSE_ERROR_TIMEOUT,
  HCD_RESPONSE_ERROR_UNKNOWN,
  HCD_RESPONSE_ERROR_INCOMPATIBLE,
  HCD_RESPONSE_RETRY,
} response_t;

typedef struct {
  char* message;
} response_message_entry_t;

const char* response_error( response_t );

#endif
