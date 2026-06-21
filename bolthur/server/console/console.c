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

#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "console.h"
#include "handler.h"

/**
 * @fn void console_destroy(console_t*)
 * @brief Helper to destroy console entry
 *
 * @param console
 */
void console_destroy( console_t* console ) {
  if ( ! console ) {
    return;
  }
  // cleanup possible listeners
  queue_cleanup( console );
  // free console path
  if ( console->path ) {
    free( console->path );
  }
  // remove from handler tree
  handler_remove( console->handler );
  // free console itself
  free( console );
}

/**
 * @fn console_t* console_get_active(void)
 * @brief Helper to get active console
 *
 * @return
 */
console_t* console_get_active( void ) {
  const list_item_t* current = console_list->first;
  while ( current ) {
    console_t* found = current->data;
    if ( found->active ) {
      return found;
    }
    current = current->next;
  }
  return nullptr;
}

/**
 * @fn console_t* console_get_by_path(const char*)
 * @brief Get console by path
 *
 * @param path
 * @return
 */
console_t* console_get_by_path( const char* path ) {
  const list_item_t* current = console_list->first;
  while ( current ) {
    console_t* found = current->data;
    if ( 0 == strcmp( found->path, path ) ) {
      return found;
    }
    current = current->next;
  }
  return nullptr;
}

/**
 * @fn void console_buffer_push(console_buffer_t*, char)
 * @brief Push to buffer
 * @param buffer
 * @param c
 */
void console_buffer_push( console_buffer_t* buffer, char c ) {
  // handle buffer full
  if ( buffer->count >= MAX_BUFFER_SIZE ) {
    return;
  }
  // push into buffer
  buffer->buffer[ buffer->head ] = c;
  buffer->head = ( buffer->head + 1 ) % MAX_BUFFER_SIZE;
  buffer->count++;
}

/**
 * @fn char console_buffer_pop(console_buffer_t*)
 * @brief Pop a character from buffer
 * @param buffer
 * @return
 */
char console_buffer_pop( console_buffer_t* buffer ) {
  if ( buffer->count == 0 ) {
    return 0;
  }
  const char c = buffer->buffer[ buffer->tail ];
  buffer->tail = ( buffer->tail + 1 ) % MAX_BUFFER_SIZE;
  buffer->count--;
  return c;
}
