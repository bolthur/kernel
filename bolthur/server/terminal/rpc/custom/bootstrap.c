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
#include <stdlib.h>
#include <confini.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../../main.h"
#include "../../render.h"
#include "../../output.h"
#include "../../terminal.h"
#include "../../psf.h"
#include "../../rpc.h"
#include "../../ttf.h"
#include "../../../libconsole.h"
#include "../../../libterminal.h"

typedef struct {
  const char *target_key;
  char found_value[ 256 ];
  int found;
} ini_search_font;

typedef struct {
  const char *target_key;
  uint32_t found_value;
  int found;
} ini_search_width_height;

/**
 * @fn int ini_lookup(IniDispatch* const, void* const)
 * @brief Ini lookup callback
 * @param dispatch
 * @param user_data
 * @return
 */
static int ini_lookup( IniDispatch* const dispatch, void * const user_data ) {
  auto const search = ( ini_search_font* )user_data;
  // Check if we are matching the section and key
  if ( 0 == strcmp( dispatch->data, search->target_key ) ) {
    snprintf( search->found_value, sizeof( search->found_value ), "%s", dispatch->value );
    search->found = 1;
  }
  return 0;
}

/**
 * @fn int ini_lookup(IniDispatch* const, void* const)
 * @brief Ini lookup callback
 * @param dispatch
 * @param user_data
 * @return
 */
static int ini_lookup_width_height( IniDispatch* const dispatch, void * const user_data ) {
  auto const search = ( ini_search_width_height* )user_data;
  // Check if we are matching the section and key
  if ( 0 == strcmp( dispatch->data, search->target_key ) ) {
    search->found_value = ( uint32_t )strtoul( dispatch->value, nullptr, 10 );
    search->found = 1;
  }
  return 0;
}

/**
 * @fn void rpc_custom_bootstrap(size_t, pid_t, size_t, size_t)
 * @brief handle bootstrap request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_custom_bootstrap(
  [[maybe_unused]] size_t type,
  pid_t origin,
  size_t data_info,
  [[maybe_unused]] size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get message and data size
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate for data fetching
  auto const command = ( terminal_bootstrap_t* )request->container;
  // build search object
  ini_search_font search = { .target_key = "FONT", };
  ini_search_width_height width = { .target_key = "WIDTH" };
  ini_search_width_height height = { .target_key = "HEIGHT" };
  // load stuff font
  if ( load_ini_path( command->config, INI_DEFAULT_FORMAT, NULL, ini_lookup, &search ) ) {
    free( request );
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // load width font
  if ( load_ini_path( command->config, INI_DEFAULT_FORMAT, NULL, ini_lookup_width_height, &width ) ) {
    free( request );
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // load height font
  if ( load_ini_path( command->config, INI_DEFAULT_FORMAT, NULL, ini_lookup_width_height, &height ) ) {
    free( request );
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle not found
  if ( ! search.found || ! width.found || ! height.found ) {
    free( request );
    error.status = -ENODATA;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // initialize ttf
  if ( ! ttf_init( search.found_value, width.found_value, height.found_value ) ) {
    free( request );
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get active terminal
  char* active = terminal_get_active();
  if ( ! active ) {
    free( request );
    error.status = -EINVAL;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // destroy possible psf
  psf_destroy();
  // adjust terminals
  auto current = terminal_list->first;
  while ( current ) {
    // get terminal
    auto const term = ( terminal_t* )current->data;
    // adjust max row
    term->max_row = resolution_data.height / ttf_glyph_height();
    if ( 0 == resolution_data.height % ttf_glyph_height() ) {
      term->max_row--;
    }
    // adjust max col
    term->max_col = resolution_data.width / ttf_glyph_width();
    if ( 0 == resolution_data.width % ttf_glyph_width() ) {
      term->max_col--;
    }
    // reinit buffer
    buffer_reinit( &term->buffer, term->max_row, term->max_col );
    // render whole terminal
    if ( 0 == strcmp( term->path, active ) ) {
      render_whole_terminal( term );
    }
    // switch to next
    current = current->next;
  }
  // free all used temporary structures
  free( request );
  free( active );
  // set success flag and return
  memset( &error, 0, sizeof( error ) );
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
}
