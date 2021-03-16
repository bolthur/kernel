
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

#if ! defined( __CORE_MESSAGE__ )
#define __CORE_MESSAGE__

#include <unistd.h>
#include <stdbool.h>
#include <collection/list.h>

typedef struct {
  size_t id;
  size_t type;
  const char* data;
  size_t length;
  pid_t sender;
  size_t request;
} message_entry_t, *message_entry_ptr_t;

bool message_init( void );
size_t message_generate_id( void );

#endif
