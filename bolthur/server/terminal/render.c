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
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include <stdarg.h>
#include "render.h"
#include "terminal.h"
#include "psf.h"
#include "main.h"
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
  uint32_t off = ( start_y * pitch ) +
    ( start_x * depth / CHAR_BIT );
  uint32_t line = off;
  uint32_t bytesperline = ( font_width + 7 ) / 8;
  for ( uint32_t idx = 0; idx < font_width * font_height; idx++ ) {
    uint32_t x = idx % font_width;
    *( ( uint32_t* )( surface + line ) ) =
      ( glyph[ x / 8 ] & ( 0x80 >> ( x & 7 ) ) ) ? color_fg : color_bg;
    line += 4;
    // handle new line
    if ( 0 == ( idx + 1 ) % font_width ) {
      *( ( uint32_t* )( surface + line ) ) = 0;
      glyph += bytesperline;
      off += pitch;
      // reset line to new offset
      line = off;
    }
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
  uint32_t start_row;
  uint32_t end_row;
  uint32_t start_col;
  uint32_t end_col;
  // push to terminal
  uint32_t scrolled = terminal_push(
    term,
    s,
    &start_col,
    &start_row,
    &end_col,
    &end_row
  );

  // get some glyph information for rendering
  uint32_t font_width = psf_glyph_width();
  uint32_t font_height = psf_glyph_height();
  uint32_t byte_per_pixel = term->bpp / CHAR_BIT;

  if ( scrolled ) {
    framebuffer_scroll_ptr_t scroll_action = malloc(
      sizeof( framebuffer_scroll_t ) );
    if ( ! scroll_action ) {
      return;
    }
    // prepare rpc block
    memset( scroll_action, 0, sizeof( framebuffer_scroll_t ) );
    scroll_action->start_y = scrolled * font_height;
    // call render surface
    int result = ioctl(
      output_driver_fd,
      IOCTL_BUILD_REQUEST(
        FRAMEBUFFER_SCROLL,
        sizeof( framebuffer_scroll_t ),
        IOCTL_WRONLY
      ),
      scroll_action
    );
    // handle error
    if ( -1 == result ) {
      free( scroll_action );
      return;
    }
    // free again
    free( scroll_action );
  }

  for ( uint32_t row = start_row; row <= end_row; row++ ) {
    // get correct start and end column
    uint32_t tmp_start_col = 0;
    if ( row == start_row ) {
      tmp_start_col = start_col;
    }
    uint32_t tmp_end_col = term->max_col - 1;
    if ( row == end_row ) {
      tmp_end_col = end_col;
    }
    // column count
    size_t row_size = ( tmp_end_col - tmp_start_col + 1 ) * font_width * byte_per_pixel;
    // calculate action size
    size_t action_size = sizeof( framebuffer_render_surface_t )
        + row_size * font_height * sizeof( uint8_t );
    // allocate rpc parameter block
    framebuffer_render_surface_ptr_t action = malloc( action_size );
    if ( ! action ) {
      return;
    }
    // initialize space with 0
    memset( action, 0, action_size );
    // loop through columns
    for (
      uint32_t col = tmp_start_col, render_col = 0;
      col <= tmp_end_col;
      col++, render_col++
    ) {
      // get character to print
      uint16_t c = term->buffer[ row * term->max_col + col ];
      // render character to buffer
      render_char_to_surface(
        action->data,
        term->bpp,
        row_size,
        c,
        render_col * font_width,
        0,
        0xf0f0f0,
        0
      );
    }

    // populate necessary data
    action->x = tmp_start_col * font_width;
    action->y = row * font_height;
    action->max_x = row_size;
    action->max_y = action->max_x * font_height;
    action->bpp = term->bpp / CHAR_BIT;
    // call render surface
    int result = ioctl(
      output_driver_fd,
      IOCTL_BUILD_REQUEST(
        FRAMEBUFFER_RENDER_SURFACE,
        action_size,
        IOCTL_WRONLY
      ),
      action
    );
    // handle error
    if ( -1 == result ) {
      free( action );
      return;
    }
    // free again
    free( action );
  }

  // call render surface
  int result = ioctl(
    output_driver_fd,
    IOCTL_BUILD_REQUEST(
      FRAMEBUFFER_FLIP,
      0,
      IOCTL_NONE
    ),
    NULL
  );
  // handle error
  if ( -1 == result ) {
    return;
  }
}
