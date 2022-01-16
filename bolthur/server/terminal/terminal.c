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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include "terminal.h"
#include "output.h"
#include "collection/list.h"
#include "psf.h"
#include "utf8.h"
#include "main.h"
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
  return strcmp( ( ( terminal_ptr_t )a->data )->path, data );
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
 */
bool terminal_init( void ) {
  // construct list
  terminal_list = list_construct( terminal_lookup, terminal_cleanup );
  if ( ! terminal_list ) {
    return false;
  }
  // allocate memory for add request
  size_t msg_size = sizeof( vfs_add_request_t ) + sizeof( size_t ) * 3;
  vfs_add_request_ptr_t msg = malloc( msg_size );
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
  size_t in = RPC_CUSTOM_START;
  size_t out = RPC_CUSTOM_START + 1;
  size_t err = RPC_CUSTOM_START + 2;
  // push terminals
  for ( uint32_t current = 0; current < TERMINAL_MAX_NUM; current++ ) {
    // prepare device path
    snprintf(
      tty_path,
      TERMINAL_MAX_PATH,
      TERMINAL_BASE_PATH"%"PRIu32,
      current
    );
    // clear memory
    memset( msg, 0, sizeof( vfs_add_request_t ) );
    // prepare message structure
    msg->info.st_mode = S_IFCHR;
    strncpy( msg->file_path, tty_path, PATH_MAX - 1 );
    msg->device_info[ 0 ] = in;
    msg->device_info[ 1 ] = out;
    msg->device_info[ 2 ] = err;
    // perform add request
    send_vfs_add_request( msg, msg_size, 0 );
    // register handler for streams
    bolthur_rpc_bind( out, output_handle_out );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to bind rpc %d: %s\r\n",
        out,
        strerror( errno )
      )
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    bolthur_rpc_bind( err, output_handle_err );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to bind rpc %d: %s\r\n",
        err,
        strerror( errno )
      )
      list_destruct( terminal_list );
      free( command_add );
      free( command_select );
      free( msg );
      free( terminal_list );
      return false;
    }
    bolthur_rpc_bind( in, output_handle_in );
    if ( errno ) {
      EARLY_STARTUP_PRINT(
        "Unable to bind rpc %d: %s\r\n",
        in,
        strerror( errno )
      )
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
    strncpy( command_add->terminal, tty_path, PATH_MAX - 1 );
    command_add->in = in;
    command_add->out = out;
    command_add->err = err;
    command_add->origin = getpid();
    /*EARLY_STARTUP_PRINT( "terminal = %s, in = %d, out = %d, err = %d, origin = %d\r\n",
      command_add->terminal, in, out, err, command_add->origin )*/
    // call console add
    int result = ioctl(
      console_manager_fd,
      IOCTL_BUILD_REQUEST(
        CONSOLE_ADD,
        sizeof( console_command_add_t ),
        IOCTL_RDWR
      ),
      command_add
    );
    if ( -1 == result ) {
      free( command_add );
      free( command_select );
      free( msg );
      list_destruct( terminal_list );
      return false;
    }
    /// FIXME: WHAT ABOUT RETURN?
    //int response = *( ( int* )command_add );
    in += 3;
    out += 3;
    err += 3;
  }

  memset( command_select, 0, sizeof( console_command_select_t ) );
  // prepare structure
  strncpy( command_select->path, "/dev/tty0", PATH_MAX - 1 );
  // call console select
  int result = ioctl(
    console_manager_fd,
    IOCTL_BUILD_REQUEST(
      CONSOLE_SELECT,
      sizeof( console_command_select_t ),
      IOCTL_RDWR
    ),
    command_select
  );
  if ( -1 == result ) {
    free( command_add );
    free( command_select );
    free( msg );
    list_destruct( terminal_list );
    return false;
  }
  int response = *( ( int* )command_select );
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
  // move up one line
  memmove(
    term->buffer,
    &term->buffer[ 1 * term->max_col ],
    sizeof( uint16_t ) * ( term->max_row - 1 ) * term->max_col
  );
  // erase last one
  memset16(
    &term->buffer[ ( term->max_row - 1 ) * term->max_col ],
    ' ',
    sizeof( uint16_t ) * term->max_col / 2
  );
}

/**
 * @fn uint32_t terminal_push(terminal_ptr_t, const char*, uint32_t*, uint32_t*, uint32_t*, uint32_t*)
 * @brief Push string to terminal buffer
 *
 * @param term terminal to push to
 * @param s utf8 string to push
 * @param start_x pointer to return start_x if not null
 * @param start_y pointer to return start_y if not null
 * @param end_x pointer to return end_x if not null
 * @param end_y pointer to return end_y if not null
 * @return
 */
uint32_t terminal_push(
  terminal_ptr_t term,
  const char* s,
  uint32_t* start_x,
  uint32_t* start_y,
  uint32_t* end_x,
  uint32_t* end_y
) {
  uint32_t scrolled = 0;
  // set start x and y
  if ( start_x ) {
    *start_x = term->col;
  }
  if ( start_y ) {
    *start_y = term->row;
  }
  bool first = true;
  bool newline_in_middle = false;
  bool newline = false;
  while( *s ) {
    // set flag indicating that a newline is in the middle
    if ( newline ) {
      newline_in_middle = true;
    }
    // handle end of row reached
    if ( term->max_col <= term->col ) {
      term->col = 0;
      term->row++;
      newline = true;
    }
    // handle scroll
    if ( term->max_row <= term->row ) {
      // scroll up content
      terminal_scroll( term );
      // increase scrolled lines
      scrolled++;
      // set row and col correctly
      term->row--;
      term->col = 0;
      // set start x and y different if during first run
      if ( first && start_x ) {
        *start_x = term->col;
      }
      if ( first && start_y ) {
        *start_y = term->row;
      }
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
        newline = true;
        break;
      // carriage return reset column
      case '\r':
        // set end x before reset
        if ( end_x ) {
          *end_x = term->col - 1;
        }
        term->col = 0;
        break;
      case '\t':
        // insert 4 spaces
        if ( first ) {
          terminal_push( term, "    ", start_x, start_y, end_x, end_y );
        } else {
          terminal_push( term, "    ", NULL, NULL, end_x, end_y );
        }
        break;
      default:
        /*EARLY_STARTUP_PRINT(
          "c = %c, idx = %ld\r\n",
          c, term->row * term->max_col + term->col
        )*/
        // push back character
        term->buffer[ term->row * term->max_col + term->col ] = c;
        // set end x
        if ( end_x ) {
          *end_x = term->col;
        }
        // increment column
        term->col++;
    }

    // next character
    first = false;
    s++;
  }
  // set end x / y in case of scroll action
  if ( end_x && newline && newline_in_middle ) {
    *end_x = term->col;
  }
  if ( end_y ) {
    *end_y = ( newline && ! newline_in_middle )
      ? term->row - 1 : term->row;
  }
  // adjust start if scrolled with newline in middle
  if ( start_y && scrolled && newline && newline_in_middle ) {
    *start_y = term->row - 1;
  }
  // return indicator whether scrolling was done or not
  return scrolled;
}
