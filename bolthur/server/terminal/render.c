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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "render.h"
#include "terminal.h"
#include "psf.h"
#include "../libframebuffer.h"

/**
 * @fn void render_char_to_surface(uint8_t*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
 * @brief Helper to render character to passed surface
 *
 * @param surface
 * @param depth
 * @param pitch
 * @param c
 * @param start_x
 * @param start_y
 * @param color_fg
 * @param color_bg
 */
void render_char_to_surface(
  uint8_t* surface,
  uint32_t depth,
  uint32_t pitch,
  uint32_t c,
  uint32_t start_x,
  uint32_t start_y,
  uint32_t color_fg,
  uint32_t color_bg
) {
  // get glyph of character
  uint8_t* glyph = psf_char_to_glyph( c );
  if ( ! glyph ) {
    return;
  }
  uint32_t font_height = psf_glyph_height();
  uint32_t font_width = psf_glyph_width();
  uint32_t byte_per_pixel = depth / CHAR_BIT;
  uint32_t off = ( start_y * pitch ) +
    ( start_x * byte_per_pixel );
  uint32_t bytesperline = ( font_width + 7 ) / 8;
  uint32_t line = 0;
  uint32_t mask = 0;
  for ( uint32_t y = 0; y < font_height; y++ ) {
    line = off;
    mask = 1 << ( font_width - 1 );
    for ( uint32_t x = 0; x < font_width; x++ ) {
      *( ( uint32_t* )( surface + line ) ) =
        glyph[ x / 8 ] & ( 0x80 >> ( x & 7 ) ) ? color_fg : color_bg;
      mask >>= 1;
      line += 4;
    }
    *( ( uint32_t* )( surface + line ) ) = 0;
    glyph += bytesperline;
    off += pitch;
  }
}

/**
 * @fn void render_terminal(terminal_ptr_t, const char*)
 * @brief Internal terminal render
 *
 * @param term
 * @param s
 */
void render_terminal( terminal_ptr_t term, const char* s ) {
  // FIXME: currently only 32 bit depth is supported
  if ( 32 != term->bpp ) {
    return;
  }
  // backup row and column before
  uint32_t start_row = term->row;
  // push string to terminal backup
  bool scrolled = terminal_push( term, s );
  // gather current column and row
  uint32_t end_row = term->row;
  // get glyph information for rendering
  uint32_t font_width = psf_glyph_width();
  uint32_t font_height = psf_glyph_height();
  uint32_t byte_per_pixel = term->bpp / CHAR_BIT;
  // adjust start and end row for total render
  if ( scrolled ) {
    start_row = 0;
    end_row = term->max_row - 1;
  }

  // calculate row count, max x and max y
  uint32_t row_count = ( end_row - start_row + 1 );
  uint32_t max_x = term->max_col * font_width * byte_per_pixel;
  uint32_t max_y = row_count * max_x * font_height;
  // allocate action buffer
  size_t action_size = sizeof( framebuffer_render_surface_t )
    + max_y * sizeof( uint8_t );
  framebuffer_render_surface_ptr_t action = malloc( action_size );
  if ( ! action ) {
    return;
  }
  // initialize space with 0
  memset( action, 0, action_size );
  // loop through rows to render
  for ( uint32_t row = 0; row < row_count; row++ ) {
    // loop through columns of row to render
    for ( uint32_t col = 0; col < term->max_col; col++ ) {
      // calculate start x and y for the buffer
      uint32_t start_x = col * font_width;
      uint32_t start_y = row * font_height;
      // get character to print
      uint16_t c = term->buffer[ ( start_row + row ) * term->max_col + col ];
      // render character to buffer
      render_char_to_surface(
        action->data,
        term->bpp,
        max_x,
        c,
        start_x,
        start_y,
        0xf0f0f0,
        0
      );
    }
  }

  // populate necessary data
  action->x = 0;
  action->y = start_row * font_height;
  action->max_x = max_x;
  action->max_y = max_y;
  // call for render surface
  _rpc_raise(
    "#/dev/framebuffer#render_surface",
    6,
    action,
    action_size
  );
  // handle error
  if ( errno ) {
    free( action );
    return;
  }
  // free up space again
  free( action );
}
