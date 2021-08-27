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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "terminal.h"
#include "output.h"
#include "list.h"
#include "psf.h"
#include "utf8.h"
#include "../libconsole.h"
#include "../libhelper.h"

list_manager_ptr_t terminal_list;

/**
 * @fn int32_t terminal_lookup(const list_item_ptr_t, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t terminal_lookup(
  const list_item_ptr_t a,
  const void* data
) {
  terminal_ptr_t term = a->data;
  return strcmp( term->path, data );
}

/**
 * @fn void terminal_cleanup(const list_item_ptr_t)
 * @brief List cleanup helper
 *
 * @param a
 */
static void terminal_cleanup( const list_item_ptr_t a ) {
  terminal_ptr_t term = a->data;
  free( term->buffer );
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn void memset16*(void*, uint16_t, size_t)
 * @brief Internal memset implementation for 16 bit
 *
 * @param buf
 * @param value
 * @param size
 */
static void* memset16( void* buf, uint16_t value, size_t size ) {
  uint16_t* _buf = ( uint16_t* )buf;
  for ( size_t i = 0; i < size; i++ ) {
    _buf[ i ] = value;
  }
  return buf;
}

/**
 * @fn bool terminal_init(void)
 * @brief Generic terminal init
 *
 * @return
 *
 * @todo fetch console manager pid from vfs for rpc call
 */
bool terminal_init( void ) {
  // construct list
  terminal_list = list_construct( terminal_lookup, terminal_cleanup );
  if ( ! terminal_list ) {
    return false;
  }
  // allocate memory for add request
  vfs_add_request_ptr_t msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    list_destruct( terminal_list );
    return false;
  }
  console_command_add_ptr_t command_add = malloc( sizeof( console_command_add_t ) );
  if ( ! command_add ) {
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  console_command_select_ptr_t command_select = malloc( sizeof( console_command_select_t ) );
  if ( ! command_select ) {
    free( command_add );
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  // base path
  char tty_path[ TERMINAL_MAX_PATH ];
  char in[ TERMINAL_MAX_PATH ];
  char out[ TERMINAL_MAX_PATH ];
  char err[ TERMINAL_MAX_PATH ];
  // push terminals
  for ( uint32_t current = 0; current < TERMINAL_MAX_NUM; current++ ) {
    // prepare device path
    snprintf(
      tty_path,
      TERMINAL_MAX_PATH,
      TERMINAL_BASE_PATH"%"PRIu32,
      current
    );
    snprintf(
      in,
      TERMINAL_MAX_PATH,
      "#"TERMINAL_BASE_PATH"%"PRIu32"#in",
      current
    );
    snprintf(
      out,
      TERMINAL_MAX_PATH,
      "#"TERMINAL_BASE_PATH"%"PRIu32"#out",
      current
    );
    snprintf(
      err,
      TERMINAL_MAX_PATH,
      "#"TERMINAL_BASE_PATH"%"PRIu32"#err",
      current
    );
    // clear memory
    memset( msg, 0, sizeof( vfs_add_request_t ) );
    // prepare message structure
    msg->info.st_mode = S_IFCHR;
    strncpy( msg->file_path, tty_path, PATH_MAX );
    // perform add request
    send_add_request( msg );

    // register handler for streams
    _rpc_acquire( out, ( uintptr_t )output_handle_out );
    if ( errno ) {
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    _rpc_acquire( err, ( uintptr_t )output_handle_err );
    if ( errno ) {
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }

    // allocate internal management structure
    terminal_ptr_t term = malloc( sizeof( terminal_t ) );
    if ( ! term ) {
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    // erase allocated space
    memset( term, 0, sizeof( terminal_t ) );
    // push max columns and rows and tty path
    term->max_col = resolution_data.width / psf_glyph_width();
    term->max_row = resolution_data.height / psf_glyph_height();
    strncpy( term->path, tty_path, TERMINAL_MAX_PATH );
    term->bpp = resolution_data.depth;
    // allocate terminal buffer
    size_t buffer_size = sizeof( uint16_t ) * term->max_col * term->max_row;
    term->buffer = malloc( buffer_size );
    if ( ! term->buffer ) {
      free( term );
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    memset16( term->buffer, ' ', buffer_size / sizeof( uint16_t ) );
    // push back
    if ( ! list_push_back( terminal_list, term ) ) {
      free( term->buffer );
      free( term );
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }

    // erase
    memset( command_add, 0, sizeof( console_command_add_t ) );
    // prepare structure
    strncpy( command_add->terminal, tty_path, PATH_MAX );
    strncpy( command_add->in, in, PATH_MAX );
    strncpy( command_add->out, out, PATH_MAX );
    strncpy( command_add->err, err, PATH_MAX );
    // perform sync rpc call
    size_t response_info = _rpc_raise_wait(
      "#/dev/console#add", 4, command_add, sizeof( console_command_add_t ) );
    if ( errno ) {
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    int response;
    _rpc_get_data( ( char* )&response, sizeof( int ), response_info );
    /// FIXME: WHAT ABOUT RETURN?
  }

  memset( command_select, 0, sizeof( console_command_select_t ) );
  // prepare structure
  strncpy( command_select->path, "/dev/tty0", PATH_MAX );
  // perform sync rpc call
  size_t response_info = _rpc_raise_wait(
    "#/dev/console#select", 4,
    command_select, sizeof( console_command_select_t )
  );
  if ( errno ) {
    free( command_add );
    free( command_select );
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  int response;
  _rpc_get_data( ( char* )&response, sizeof( int ), response_info );
  // free again
  free( msg );
  free( command_add );
  free( command_select );
  // return success
  return 0 == response;
}

/**
 * @fn void terminal_scroll(terminal_ptr_t)
 * @brief Scroll up terminal buffer
 *
 * @param term
 */
void terminal_scroll( terminal_ptr_t term ) {
  // move up line by line
  for ( uint32_t row = 1; row < term->max_row - 1; row++ ) {
    memmove(
      &term->buffer[ ( row - 1 ) * term->max_row ],
      &term->buffer[ row * term->max_row ],
      sizeof( uint8_t ) * term->max_col
   );
  }
  // erase last one
  memset16(
    &term->buffer[ term->max_row - 1 ],
    ' ',
    sizeof( uint8_t ) * term->max_col
  );
}

/**
 * @fn bool terminal_push(terminal_ptr_t, const char*)
 * @brief Push string to terminal buffer
 *
 * @param term
 * @param s
 * @return
 */
bool terminal_push( terminal_ptr_t term, const char* s ) {
  bool scrolled = false;
  while( *s ) {
    // handle scroll
    if ( term->max_row <= term->row ) {
      // scroll up content
      terminal_scroll( term );
      // set flag for rendering
      scrolled = true;
      // set row and col correctly
      term->row--;
      term->col = 0;
    }
    // handle end of row reached
    if ( term->max_col <= term->col ) {
      term->col = 0;
      term->row++;
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
        break;
      default:
        // push back character
        term->buffer[ term->row * term->max_col + term->col ] = c;
        // increment column
        term->col++;
    }

    // next character
    s++;
  }
  return scrolled;
}
