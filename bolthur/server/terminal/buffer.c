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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "buffer.h"

/**
 * @fn void buffer_init(circular_line_buffer_t*, uint32_t, uint32_t)
 * @brief Initialize circular line buffer
 * @param buffer buffer to initialize
 * @param rows max columns
 * @param cols max rows
 */
void buffer_init( circular_line_buffer_t* buffer, const uint32_t rows, const uint32_t cols ) {
  // assert valid buffer
  assert( buffer );
  // initialize buffer by setting mask, head and tail
  buffer->mask = rows - 1;
  buffer->head = 0;
  buffer->tail = 0;
  buffer->rows = rows;
  buffer->columns = cols;
  // allocate buffer
  buffer->data = malloc( ( rows * cols ) * sizeof( uint16_t ) );
  assert( buffer->data );
  memset( buffer->data, 0, sizeof( uint16_t ) * ( rows * cols ) );
  // allocate length array
  buffer->len = calloc( rows, sizeof( uint32_t ) );
  assert( buffer->len );
  memset( buffer->len, 0, rows * sizeof( uint32_t ) );
}

/**
 * @fn void buffer_reinit(circular_line_buffer_t*, uint32_t, uint32_t)
 * @brief Reinitialize buffer
 * @param buffer buffer to reinitialize
 * @param rows new amount of rows
 * @param cols new amount of columns
 */
void buffer_reinit( circular_line_buffer_t* buffer, const uint32_t rows, const uint32_t cols ) {
  // assert valid buffer
  assert( buffer );
  // save old data
  auto const old_rows = buffer->rows;
  auto const old_cols = buffer->columns;
  auto const old_data = buffer->data;
  auto const old_len = buffer->len;
  // initialize buffer by setting mask, head and tail
  buffer->mask = rows - 1;
  buffer->head = 0;
  buffer->tail = 0;
  buffer->rows = rows;
  buffer->columns = cols;
  // allocate buffer
  buffer->data = malloc( ( rows * cols ) * sizeof( uint16_t ) );
  assert( buffer->data );
  memset( buffer->data, 0, sizeof( uint16_t ) * ( rows * cols ) );
  // allocate length array
  buffer->len = calloc( rows, sizeof( uint32_t ) );
  assert( buffer->len );
  memset( buffer->len, 0, rows * sizeof( uint32_t ) );
  // push to buffer
  for ( uint32_t row = 0; row < old_rows; row++ ) {
    buffer_push( buffer, &old_data[ row * old_cols ], old_len[ row ] );
  }
  // free old data
  free( old_data );
  free( old_len );
}

/**
 * @fn void buffer_destroy(circular_line_buffer_t*)
 * @brief Destroy circular line buffer
 * @param buffer buffer to destroy
 */
void buffer_destroy( circular_line_buffer_t* buffer ) {
  assert( buffer );
  if ( buffer->data ) {
    free( buffer->data );
    buffer->data = nullptr;
  }
  if ( buffer->len ) {
    free( buffer->len );
    buffer->len = nullptr;
  }
}

/**
 * @fn bool buffer_is_empty(const circular_line_buffer_t*)
 * @brief Helper to check if buffer is empty
 * @param buffer
 * @return
 */
bool buffer_is_empty( const circular_line_buffer_t* buffer ) {
  return buffer->head == buffer->tail;
}

/**
 * @fn bool buffer_is_full(const circular_line_buffer_t*)
 * @brief Helper to check if buffer is full
 * @param buffer
 * @return
 */
bool buffer_is_full( const circular_line_buffer_t* buffer ) {
  const uint32_t next_head = buffer->head == buffer->mask ? 0 : buffer->head + 1;
  return next_head == buffer->tail;
}

/**
 * @fn void buffer_push(circular_line_buffer_t*, const uint16_t*, size_t)
 * @brief Push data to buffer
 * @param buffer buffer to push data to
 * @param data data to push
 * @param length data length
 */
void buffer_push( circular_line_buffer_t* buffer, const uint16_t* data, const size_t length ) {
  // handle invalid data
  if ( ! buffer || ! data || ! length ) {
    return;
  }
  // calculate "lines" to push
  size_t lines = length / buffer->columns;
  if ( length % buffer->columns ) {
    lines++;
  }
  for ( size_t line = 0; line < lines; line++ ) {
    // in case buffer is full => continue with oldest one
    if ( buffer_is_full( buffer ) ) {
      // increase tail
      if (buffer->tail == buffer->mask) {
        buffer->tail = 0;
      } else {
        buffer->tail++;
      }
    }
    // evaluate columns to copy
    size_t cols = buffer->columns;
    if ( line + 1 == lines ) {
      cols = length % buffer->columns;
    }
    // evaluate destination and source address
    uint16_t* dest = &buffer->data[ buffer->head * buffer->columns ];
    const uint16_t* src = &data[ line * buffer->columns ];
    // copy over data
    memcpy( dest, src, cols * sizeof( uint16_t ) );
    // push length
    buffer->len[ buffer->head ] = cols;
    // fill up
    if ( cols < buffer->columns ) {
      memset( &dest[ cols ], 0, ( buffer->columns - cols ) * sizeof( uint16_t ) );
    }
    // push head pointer further
    if (buffer->head == buffer->mask) {
      buffer->head = 0;
    } else {
      buffer->head++;
    }
  }
}

/**
 * @fn uint16_t* buffer_last_pushed_data(const circular_line_buffer_t*)
 * @brief Get pointer to last pushed data
 * @param buffer
 * @return
 */
uint16_t* buffer_last_pushed_data( const circular_line_buffer_t* buffer ) {
  if ( buffer->head == buffer->tail ) {
    return nullptr;
  }
  uint32_t last_index = 0;
  if ( ! buffer->head ) {
    last_index = buffer->mask;
  } else {
    last_index = buffer->head - 1;
  }
  return &buffer->data[ last_index * buffer->columns ];
}
