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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/bolthur.h>
#include "render.h"
#include "terminal.h"
#include "psf.h"
#include "main.h"
#include "output.h"
#include "utf8.h"
#include "../libframebuffer.h"

static uint32_t foreground_color = 0xf0f0f0;
static uint32_t background_color = 0;

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
 * @fn uint32_t terminal_evaluate_foreground_color(const uint32_t)
 * @brief Method to evaluate foreground color
 * @param color
 * @return
 */
static uint32_t terminal_evaluate_foreground_color( const uint32_t color ) {
  switch (color) {
    case 30: return 0;
    case 31: return 0xaa0000;
    case 32: return 0x00aa00;
    case 33: return 0xe5e510;
    case 34: return 0x0000aa;
    case 35: return 0xaa00aa;
    case 36: return 0x00aaaa;
    case 37: return 0xf0f0f0;
    default: return foreground_color;
  }
}

/**
 * @fn uint32_t terminal_evaluate_background_color(const uint32_t)
 * @brief Method to evaluate background color
 * @param color
 * @return
 */
static uint32_t terminal_evaluate_background_color( const uint32_t color ) {
  switch (color) {
    case 40: return 0;
    case 41: return 0xaa0000;
    case 42: return 0x00aa00;
    case 43: return 0xe5e510;
    case 44: return 0x0000aa;
    case 45: return 0xaa00aa;
    case 46: return 0x00aaaa;
    case 47: return 0xf0f0f0;
    default: return background_color;
  }
}

/**
 * @fn uint32_t terminal_push(terminal_t*, const char*)
 * @brief Push string to terminal buffer
 *
 * @param term terminal to push to
 * @param s utf8 string to push
 * @return
 */
static uint32_t terminal_push( terminal_t* term, char* s ) {
  uint32_t rendered = 0;
  while( *s ) {
    if ( *s == '\x1b' && s[ 1 ] == '[' ) {
      // skip control character and opening brackets
      s += 2;
      // loop while end is reached
      char* end = s;
      while ( *end && *end != 'm' && *end != ',' ) {
        end++;
      }
      // set terminating flag
      const bool terminating = *end == 'm';
      // convert to unsigned integer
      uint32_t color = ( uint32_t )strtoul( s, &s, 10 );
      // skip separator
      s++;
      // evaluate color
      foreground_color = terminal_evaluate_foreground_color( color );
      background_color = terminal_evaluate_background_color( color );
      // handle reset
      if ( terminating && 0 == color ) {
        foreground_color = terminal_evaluate_foreground_color( 37 );
        background_color = terminal_evaluate_background_color( 40 );
      }
      // handle not yet terminating
      if ( ! terminating ) {
        end = s;
        // loop until end
        while ( *end && *end != 'm' ) {
          end++;
        }
        // convert to unsigned integer
        color = ( uint32_t )strtoul( s, &s, 10 );
        // evaluate color
        foreground_color = terminal_evaluate_foreground_color( color );
        background_color = terminal_evaluate_background_color( color );
        // skip terminating sequence
        s++;
      }
    }
    // handle delete by reducing column if greater 0
    if ( '\b' == *s || 0x7f == *s ) {
      if ( term->col > 0 ) {
        term->col--;
      }
    }
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
    // decode current character to utf8 for save
    size_t len = 0;
    const uint16_t c = utf8_decode( s, &len );
    s += --len;
    // check character for actions
    switch ( c ) {
      // newline, increase row and reset column
      case '\n':
        term->col = 0;
        term->row++;
        break;
      // carriage return reset column
      case '\r':
        term->col = 0;
        break;
      // handle tab
      case '\t':
        // insert 4 spaces
        terminal_push( term, "    " );
        ++rendered;
        break;
      // handle backspace by overwriting character with space
      case '\b':
      case 0x7f:
        terminal_push( term, " " );
        if ( term->col > 0 ) {
          term->col--;
        }
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
          foreground_color,
          background_color
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
  auto const p = ( char* )s;
  // push to terminal
  const uint32_t character_rendered = terminal_push( term, p );

  // allocate rpc parameter block
  framebuffer_surface_render_t* action = malloc( sizeof( *action ) );
  if ( ! action ) {
    return -ENOMEM;
  }
  // initialize space with 0
  memset( action, 0, sizeof( *action ) );
  // populate
  action->surface_id = term->surface_id;
  action->x = 0;
  action->y = 0;
  // call render surface
  const int result = ioctl(
    output_driver_fd,
    IOCTL_BUILD_REQUEST(
      FRAMEBUFFER_SURFACE_RENDER,
      sizeof( *action ),
      IOCTL_WRONLY
    ),
    action
  );
  free( action );
  // handle error
  if ( -1 == result ) {
    return -EIO;
  }
  // return rendered character
  return ( ssize_t )character_rendered;
}
