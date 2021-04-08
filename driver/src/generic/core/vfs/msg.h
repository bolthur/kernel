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

#include <sys/bolthur.h>

#if !defined( __MSG_H__ )
#define __MSG_H__

typedef void ( *msg_callback_t )( void );
struct msg_command_handler {
  vfs_message_type_t type;
  msg_callback_t handler;
};
typedef struct msg_command_handler msg_command_handler_t;
typedef struct msg_command_handler *msg_command_handler_ptr_t;

void msg_dispatch( vfs_message_type_t );
void msg_handle_add( void );
void msg_handle_remove( void );
void msg_handle_open( void );
void msg_handle_close( void );
void msg_handle_read( void );
void msg_handle_write( void );
void msg_handle_seek( void );
void msg_handle_size( void );
void msg_handle_has( void );

#endif
