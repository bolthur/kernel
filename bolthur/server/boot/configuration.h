/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <stdbool.h>
#include <sys/queue.h>
#include <confini.h>

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

typedef struct configuration_node {
  // data
  char name[ PATH_MAX ];
  char path[ PATH_MAX ];
  char device[ PATH_MAX ];
  bool early;
  bool reroute;
  // list stuff
  TAILQ_ENTRY( configuration_node ) queue;
} configuration_node_t;

configuration_node_t* by_name( const char* name );
int configuration_confini_handler( IniDispatch*, void* );
bool configuration_handle( const char* path );

#endif
