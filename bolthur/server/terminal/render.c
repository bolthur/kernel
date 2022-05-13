/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "output.h"
#include "utf8.h"
#include "../libframebuffer.h"


/**
 * @fn void terminal_scroll(terminal_t*)
 * @brief Scroll up terminal buffer
 *
 * @param term
 */
static void terminal_scroll( terminal_t* term ) {
  // calculate offset and size
  uint32_t offset = psf_glyph_height() * term->pitch;
  uint32_t size = resolution_data.height * term->pitch;
  // move up and reset last line
  memmove( term->surface, term->surface + offset, size - offset );
  memset( term->surface + size - offset, 0, offset );
}

/**
 * @fn uint32_t terminal_push(terminal_t*, const char*)
 * @brief Push string to terminal buffer
 *
 * @param term terminal to push to
 * @param s utf8 string to push
 * @return
 */
__unused static uint32_t terminal_push(
  terminal_t* term,
  const char* s
) {
  uint32_t rendered = 0;
  while( *s ) {
    // handle end of row reached
    if ( term->max_col <= term->col ) {
      term->col = 0;
      term->row++;
    }
    // handle scroll
    if ( term->max_row <= term->row ) {
      // scroll up content
      terminal_scroll( term );
      // set row and col correctly
      term->row--;
      term->col = 0;
    }
    // decode churrent character to unicode for save
    size_t len = 0;
    uint16_t c = utf8_decode( s, &len );
    s += --len;
    // check character for actions
    switch ( c ) {
      // newline just increase row
      case '\n':
        term->row++;
        break;
      // carriage return reset column
      case '\r':
        term->col = 0;
        break;
      case '\t':
        // insert 4 spaces
        terminal_push( term, "    " );
        ++rendered;
        break;
      default:
        // render to surface
        render_char_to_surface(
          term->surface,
          term->bpp,
          term->pitch,
          c,
          term->col * psf_glyph_width(),
          term->row * psf_glyph_height(),
          0xf0f0f0,
          0
        );
        // increment column
        term->col++;
        rendered++;
    }

    // next character
    s++;
  }

  return rendered;
}

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
  volatile uint8_t* surface,
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
    ( start_x * ( depth / CHAR_BIT ) );
  uint32_t line = off;
  uint32_t bytesperline = ( font_width + 7 ) / 8;
  uint32_t idx = 0;
  uint32_t max = font_width * font_height;
  while( idx < max ) {
    uint32_t x = idx % font_width;
    *( ( uint32_t* )( surface + line ) ) =
      ( glyph[ x / 8 ] & ( 0x80 >> ( x & 7 ) ) ) ? color_fg : color_bg;
    line += 4;
    idx++;
    // handle new line
    if ( 0 == idx % font_width && idx < max ) {
      *( ( uint32_t* )( surface + line ) ) = 0;
      glyph += bytesperline;
      off += pitch;
      // reset line to new offset
      line = off;
    }
  }
}

/**
 * @fn void render_terminal(terminal_t*, const char*)
 * @brief Internal terminal render
 *
 * @param term
 * @param s
 * @return rendered character length
 */
ssize_t render_terminal( terminal_t* term, const char* s ) {
  // FIXME: currently only 32 bit depth is supported
  if ( 32 != term->bpp ) {
    return -ENOSYS;
  }
  // push to terminal
  uint32_t character_rendered = terminal_push( term, s );

  // allocate rpc parameter block
  framebuffer_surface_render_t* action = malloc(
    sizeof( framebuffer_surface_render_t ) );
  if ( ! action ) {
    return -ENOMEM;
  }
  // initialize space with 0
  memset( action, 0, sizeof( framebuffer_surface_render_t ) );
  // populate
  action->surface_id = term->surface_id;
  action->x = 0;
  action->y = 0;
  // call render surface
  int result = ioctl(
    output_driver_fd,
    IOCTL_BUILD_REQUEST(
      FRAMEBUFFER_SURFACE_RENDER,
      sizeof( framebuffer_surface_render_t ),
      IOCTL_WRONLY
    ),
    action
  );
  // handle error
  if ( -1 == result ) {
    free( action );
    return -EIO;
  }
  // call render surface
  result = ioctl(
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
    free( action );
    return -EIO;
  }
  return ( ssize_t )character_rendered;
}
