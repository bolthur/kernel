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

#include <string.h>
#include <ini.h>
#include <stdlib.h>
#include "server.h"

/**
 * @brief ini parser callback
 * @param user_ptr
 * @param section
 * @param name
 * @param value
 * @return
 */
static int config_handler(
  void* user_ptr,
  const char* section,
  const char* name,
  const char* value
) {
  auto const ini = ( server_ini_t* )user_ptr;
  // only general section is supported
  if ( 0 == strcmp( section, "general" ) ) {
    if ( 0 == strcmp( name, "path" ) ) {
      strncpy( ini->path, value, sizeof( ini->path ) - 1 );
    } else if ( 0 == strcmp( name, "type" ) ) {
      strncpy( ini->type, value, sizeof( ini->type ) - 1 );
    }
  }
  return 1;
}

/**
 * @brief Helper to parse server ini into c structure
 * @param path
 * @return
 */
server_ini_t* server_parse_ini( const char* path ) {
  server_ini_t* ini = malloc( sizeof( *ini ) );
  if ( ! ini ) {
    return nullptr;
  }
  memset( ini, 0, sizeof( *ini ) );
  if ( 0 > ini_parse( path, config_handler, ini ) ) {
    free( ini );
    return nullptr;
  }
  return ini;
}
