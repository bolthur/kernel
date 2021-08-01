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

#include <unistd.h>
#include <stdbool.h>
#include <collection/list.h>
#include <task/process.h>

#if ! defined( __CORE_IPC_MESSAGE__ )
#define __CORE_IPC_MESSAGE__

struct message_entry {
  size_t id;
  size_t type;
  const char* data;
  size_t length;
  pid_t sender;
  size_t request;
};

typedef struct message_entry message_entry_t;
typedef struct message_entry *message_entry_ptr_t;

bool message_init( void );
void message_cleanup( const list_item_ptr_t );
size_t message_generate_id( void );

bool message_setup_process( task_process_ptr_t );
void message_destroy_process( task_process_ptr_t );
int message_send_by_pid( pid_t, pid_t, size_t, const char*, size_t, size_t, size_t* );
int message_send_by_name( const char*, pid_t, size_t, const char*, size_t, size_t, size_t* );

#endif
