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

#ifndef _QUEUE_H
#define _QUEUE_H

#include <sys/bolthur.h>
#include <sys/queue.h>
#include "handler.h"

typedef struct queue_node {
  size_t return_type;
  size_t response_info;
  vfs_read_request_t* request;
  size_t read_amount;
  handler_node_t* handler;
  TAILQ_ENTRY( queue_node ) node;
} queue_node_t;

typedef TAILQ_HEAD( queue_head, queue_node ) queue_head_t;

// generic stuff
bool queue_setup( void );
bool queue_push( size_t, size_t, vfs_read_request_t*, handler_node_t* );
void queue_handle( const char*, const char* );

#endif
