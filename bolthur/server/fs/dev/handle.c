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

#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "handle.h"

static list_manager_t* device_list;

/**
 * @fn void destroy_device(device_handle_t*)
 * @brief Helper to destroy device entry
 *
 * @param device
 */
static void destroy_device( device_handle_t* device ) {
  if ( ! device ) {
    return;
  }
  if ( device->path ) {
    free( device->path );
  }
  free( device );
}

/**
 * @fn int32_t console_lookup(const list_item_t*, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t handle_lookup(
  const list_item_t* a,
  const void* data
) {
  device_handle_t* device = a->data;
  return strcmp( device->path, data );
}

/**
 * @fn bool handle_insert(list_manager_t*, void*)
 * @brief Device insert callback
 *
 * @param list
 * @param data
 * @return
 */
static bool handle_insert( list_manager_t* list, void* data ) {
  // loop until there is a matching point
  list_item_t* item = list->first;
  while ( item && 0 > handle_lookup( item, data ) ) {
    item = item->next;
  }
  // insert before last item
  if ( item ) {
    return list_insert_data_before( list, item, data );
  }
  // default insert at the end
  return list_push_back_data( list, data );
}

/**
 * @fn void console_cleanup(list_item_t*)
 * @brief List cleanup helper
 *
 * @param a
 */
static void handle_cleanup( list_item_t* a ) {
  destroy_device( a->data );
  list_default_cleanup( a );
}

/**
 * @fn bool handle_init(void)
 * @brief init device handling structure
 *
 * @return
 */
bool handle_init( void ) {
  // construct device management list
  device_list = list_construct( handle_lookup, handle_cleanup, handle_insert );
  if ( ! device_list ) {
    return false;
  }
  // return success
  return true;
}

/**
 * @fn device_handle_t* handle_get_by_path(const char*)
 * @brief Method to get device by path
 *
 * @param name
 * @return
 */
device_handle_t* handle_get_by_path( const char* path ) {
  list_item_t* item = list_lookup_data( device_list, ( void* )path );
  return item ? item->data : NULL;
}

/**
 * @fn device_handle_t* handle_get_by_id(const char*)
 * @brief Method to get device by path
 *
 * @param name
 * @return
 */
device_handle_t* handle_get_by_id( pid_t id ) {
  list_item_t* item = device_list->first;
  while ( item ) {
    // check for matching id
    device_handle_t* possible = item->data;
    if ( possible->process == id ) {
      break;
    }
    // get next item
    item = item->next;
  }
  // return found item or null
  return item ? item->data : NULL;
}

/**
 * @fn bool handle_add(const char*, struct stat, pid_t)
 * @brief Method to add device
 *
 * @param path
 * @param info
 * @param process
 * @return
 */
bool handle_add( const char* path, struct stat info, pid_t process ) {
  // return false if existing
  if ( handle_get_by_path( path ) ) {
    return false;
  }
  // create device
  device_handle_t* device = malloc( sizeof( *device ) );
  if ( ! device ) {
    return false;
  }
  // clear out
  memset( device, 0, sizeof( *device ) );
  // populate structure
  device->path = strdup( path );
  if ( ! device->path ) {
    destroy_device( device );
    return false;
  }
  device->process = process;
  memcpy( &device->info, &info, sizeof( info ) );
  // try to insert
  if ( ! list_insert_data( device_list, device ) ) {
    destroy_device( device );
    return false;
  }
  // return success
  return true;
}
