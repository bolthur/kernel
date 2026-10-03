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
#include <assert.h>
#include "render.h"

#include "buffer.h"
#include "terminal.h"
#include "psf.h"
#include "ttf.h"
#include "main.h"
#include "output.h"
#include "utf8.h"
#include "../libframebuffer.h"

static uint32_t foreground_color = 0xf0f0f0;
static uint32_t background_color = 0;

/**
 * @fn void terminal_scroll(void)
 * @brief Scroll up terminal buffer
 */
static void terminal_scroll( void ) {
  // calculate offset and size
  uint32_t offset = 0;
  if ( psf_initialized() ) {
    offset = psf_glyph_height() * surface_data.pitch;
  } else if ( ttf_initialized() ) {
    offset = ttf_glyph_height() * surface_data.pitch;
  }
  assert( offset > 0 );
  const uint32_t size = resolution_data.height * surface_data.pitch;
  // move up and reset last line
  memmove( surface, surface + offset, size - offset );
  memset( surface + size - offset, 0, offset );
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

static void terminal_push_utf8( terminal_t* term, uint16_t* s ) {
  while( *s ) {
    if ( *s == '\x1b' && s[ 1 ] == '[' ) {
      // skip control character and opening brackets
      s += 2;
      // loop while end is reached
      auto end = s;
      auto str = s;
      while ( *end && *end != 'm' && *end != ',' ) {
        end++;
      }
      // set terminating flag
      const bool terminating = *end == 'm';
      // convert into string
      size_t size = ( size_t )( end - str );
      char* cs = malloc( ( size + 1 ) * sizeof( char ) );
      if ( cs ) {
        for ( size_t i = 0; i < size; i++ ) {
          cs[ i ] = ( char )str[ i ];
        }
        cs[ size ] = '\0';
        // skip size and separator
        s += ( size + 1 );
        // convert to unsigned integer
        uint32_t color = ( uint32_t )strtoul( cs, nullptr, 10 );
        // evaluate color
        foreground_color = terminal_evaluate_foreground_color( color );
        background_color = terminal_evaluate_background_color( color );
        // handle reset
        if ( terminating && 0 == color ) {
          foreground_color = terminal_evaluate_foreground_color( 37 );
          background_color = terminal_evaluate_background_color( 40 );
        }
        free( cs );
        // handle not yet terminating
        if ( ! terminating ) {
          end = s;
          // loop until end
          while ( *end && *end != 'm' ) {
            end++;
          }
          // calculate size
          size = ( size_t )( end - str );
          cs = malloc( ( size + 1 ) * sizeof( char ) );
          if ( cs ) {
            for ( size_t i = 0; i < size; i++ ) {
              cs[ i ] = ( char )str[ i ];
            }
            cs[ size ] = '\0';
            // convert to unsigned integer
            color = ( uint32_t )strtoul( cs, nullptr, 10 );
            // evaluate color
            foreground_color = terminal_evaluate_foreground_color( color );
            background_color = terminal_evaluate_background_color( color );
            // skip terminating sequence
            str += ( size + 1 );
            free( cs );
          }
        }
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
      terminal_scroll();
      // set row and col correctly
      term->row--;
      term->col = 0;
    }
    // check character for actions
    switch ( *s ) {
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
        size_t tab = 0;
        uint16_t* tb = utf8_decode_string( "    ", &tab );
        if ( tb ) {
          terminal_push_utf8( term, tb );
          free( tb );
        }
        break;
      // handle backspace by overwriting character with space
      case '\b':
      case 0x7f:
        size_t backspace = 0;
        uint16_t* bsp = utf8_decode_string( " ", &backspace );
        if ( bsp ) {
          terminal_push_utf8( term, bsp );
          free( bsp );
        }
        if ( term->col > 0 ) {
          term->col--;
        }
        break;
      default:
        // render to surface via psf if initialized
        if ( psf_initialized() ) {
          psf_render_char(
            surface,
            term->bpp,
            surface_data.pitch,
            *s,
            term->col,
            term->row,
            foreground_color,
            background_color
          );
        } else if ( ttf_initialized() ) {
          ttf_render_char(
            surface,
            term->bpp,
            surface_data.pitch,
            *s,
            term->col,
            term->row,
            foreground_color,
            background_color
          );
        }
        // increment column
        term->col++;
    }
    // next character
    s++;
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
int render_terminal( terminal_t* term, const char* s ) {
  // FIXME: currently only 32 bit depth is supported
  if ( 32 != term->bpp ) {
    return -ENOSYS;
  }
  // decode utf8
  size_t len = 0;
  uint16_t* line = utf8_decode_string( s, &len );
  if ( ! line ) {
    return -ENOMEM;
  }
  // push to buffer
  buffer_push( &term->buffer, line, len );
  // get active terminal
  char* active = terminal_get_active();
  if ( ! active ) {
    return -ENOMEM;
  }
  // push current utf8 line to terminal if active
  if ( 0 != strcmp( active, term->path ) ) {
    free( active );
    return 0;
  }
  free( active );
  // push to terminal
  terminal_push_utf8( term, buffer_last_pushed_data( &term->buffer ) );
  // allocate rpc parameter block
  framebuffer_surface_render_t* action = malloc( sizeof( *action ) );
  if ( ! action ) {
    return -ENOMEM;
  }
  // initialize space with 0
  memset( action, 0, sizeof( *action ) );
  // populate
  action->surface_id = surface_data.surface_id;
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
  return 0;
}

/**
 * @fn void render_whole_terminal(terminal_t*)
 * @brief Wrapper to render whole terminal
 * @param term
 */
void render_whole_terminal( terminal_t* term ) {
  // clear previous terminal
  const uint32_t size = resolution_data.height * surface_data.pitch;
  memset( surface, 0, size );
  // reset col and row
  term->row = term->col = 0;
  // rerender whole terminal
  uint32_t current = term->buffer.tail;
  while ( current != term->buffer.head ) {
    uint16_t* line_ptr = &term->buffer.data[ current * term->buffer.columns ];
    // push utf8 to terminal
    terminal_push_utf8( term, line_ptr );
    // get to next line
    if ( current == term->buffer.mask ) {
      current = 0;
    } else {
      current++;
    }
  }
  // allocate rpc parameter block
  framebuffer_surface_render_t* action = malloc( sizeof( *action ) );
  if ( ! action ) {
    return;
  }
  // initialize space with 0
  memset( action, 0, sizeof( *action ) );
  // populate
  action->surface_id = surface_data.surface_id;
  action->x = 0;
  action->y = 0;
  // call render surface
  ioctl(
    output_driver_fd,
    IOCTL_BUILD_REQUEST(
      FRAMEBUFFER_SURFACE_RENDER,
      sizeof( *action ),
      IOCTL_WRONLY
    ),
    action
  );
  free( action );
}
