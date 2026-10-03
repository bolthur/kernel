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

#ifndef _BUFFER_H
#define _BUFFER_H

/**
 * @brief Circular buffer implementation
 */
typedef struct circular_line_buffer {
  /** utf8 data to be rendered */
  uint16_t* data;
  /** array with len per row */
  uint32_t* len;
  /** rows */
  uint32_t rows;
  /** columns */
  uint32_t columns;
  /** head index */
  uint32_t head;
  /** tail index */
  uint32_t tail;
} circular_line_buffer_t;

void buffer_init( circular_line_buffer_t*, uint32_t, uint32_t );
void buffer_reinit( circular_line_buffer_t*, uint32_t, uint32_t );
void buffer_destroy( circular_line_buffer_t* );
bool buffer_is_empty( const circular_line_buffer_t* );
bool buffer_is_full( const circular_line_buffer_t* );
void buffer_push( circular_line_buffer_t* , const uint16_t*, size_t );
uint16_t* buffer_last_pushed_data( const circular_line_buffer_t* );

#endif
