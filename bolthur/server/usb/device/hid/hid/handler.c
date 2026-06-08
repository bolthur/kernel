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
#include <string.h>
#include <sys/bolthur.h>
#include "handler.h"
#include "../../../../libusbd.h"
#include "../../../../../library/collection/avl/avl.h"

static avl_tree_t* management_tree;


/**
 * @fn int32_t compare_container(const avl_node_t*, const avl_node_t*)
 * @brief Compare handle callback necessary for avl tree insert / delete
 *
 * @param node_a
 * @param node_b
 * @return
 */
static int32_t compare_container(
  const avl_node_t* node_a,
  const avl_node_t* node_b
) {
  const libusb_hid_usage_page_desktop_t value_a = (libusb_hid_usage_page_desktop_t)node_a->data;
  const libusb_hid_usage_page_desktop_t value_b = (libusb_hid_usage_page_desktop_t)node_b->data;
  // return 0 if equal
  if ( value_a == value_b ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return value_a > value_b ? -1 : 1;
}

/**
 * @fn int32_t lookup_container(const avl_node_t*, const void*)
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node
 * @param value
 * @return
 */
static int32_t lookup_container(
  const avl_node_t* node,
  const void* value
) {
  const libusb_hid_usage_page_desktop_t type = ( libusb_hid_usage_page_desktop_t )value;
  const libusb_hid_usage_page_desktop_t node_type = ( libusb_hid_usage_page_desktop_t )node->data;
  // return 0 if equal
  if ( node_type == type ) {
    return 0;
  }
  // return -1 or 1 depending on what is greater
  return node_type > type ? -1 : 1;
}

/**
 * @fn void cleanup_container(avl_node_t*)
 * @brief handle cleanup
 *
 * @param node
 */
static void cleanup_container( avl_node_t* node ) {
  pid_container_t* item = PID_HANDLER_GET_ENTRY( node );
  // free item
  free( item );
}

/**
 * @fn int handler_init(void)
 * @brief Wrapper to init handler management
 * @return
 */
int handler_init( void ) {
  // generate tree
  management_tree = avl_create_tree(
    compare_container,
    lookup_container,
    cleanup_container
  );
  // handle error
  if ( ! management_tree ) {
    return ENOMEM;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_register(libusb_hid_usage_page_desktop_t, pid_t)
 * @brief Method to register a handler
 * @param type
 * @param handler
 * @return
 */
int handler_register( const libusb_hid_usage_page_desktop_t type, const pid_t handler ) {
  // validate
  if ( ! management_tree ) {
    return EINVAL;
  }
  // try to find possible handler
  const avl_node_t* found = avl_find_by_data( management_tree, ( void* )type );
  // handle found
  if ( found ) {
    return EINVAL;
  }
  // allocate new container
  pid_container_t* item = malloc( sizeof( *item ) );
  if ( ! item ) {
    return ENOMEM;
  }
  // clear out
  memset( item, 0, sizeof( *item ) );
  // pure in data
  item->handler = handler;
  // prepare node
  avl_prepare_node( &item->node, ( void* )type );
  // insert into tree
  if ( ! avl_insert_by_node( management_tree, &item->node ) ) {
    free( item );
    return EAGAIN;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_unregister(libusb_hid_usage_page_desktop_t, pid_t)
 * @brief Unregister a handler
 * @param type
 * @param handler
 * @return
 */
int handler_unregister( const libusb_hid_usage_page_desktop_t type, const pid_t handler ) {
  // validate
  if ( ! management_tree ) {
    return EINVAL;
  }
  // try to find possible handler
  avl_node_t* found = avl_find_by_data( management_tree, ( void* )type );
  // handle not found => return success
  if ( ! found ) {
    return 0;
  }
  // compare handlers
  pid_container_t* item = PID_HANDLER_GET_ENTRY( found );
  // handle no match
  if ( item->handler != handler ) {
    return EINVAL;
  }
  // return node
  avl_remove_by_node( management_tree, found );
  // cleanup node
  free( item );
  // return success
  return 0;
}

/**
 * @fn int handler_get(libusb_hid_usage_page_desktop_t, pid_t*)
 * @brief Get a handler
 * @param type
 * @param handler
 * @return
 */
int handler_get( const libusb_hid_usage_page_desktop_t type, pid_t* handler ) {
  // validate
  if ( ! handler || ! management_tree ) {
    return EINVAL;
  }
  // try to find possible handler
  const avl_node_t* found = avl_find_by_data( management_tree, ( void* )type );
  // handle not found
  if ( ! found ) {
    *handler = -1;
  // handle found
  } else {
    const pid_container_t* container = PID_HANDLER_GET_ENTRY( found );
    *handler = container->handler;
  }
  // return success
  return 0;
}

/**
 * @fn int handler_call_attach(libusb_hid_usage_page_desktop_t, libusb_hid_device_t*, uint32_t, uint32_t, rpc_handler_t, pid_t, size_t)
 * @brief Wrapper to call attach
 * @param type
 * @param device
 * @param device_number
 * @param interface_number
 * @param callback
 * @param origin
 * @param data_info
 * @return
 */
int handler_call_attach(
  const libusb_hid_usage_page_desktop_t type,
  libusb_hid_device_t* device,
  const uint32_t device_number,
  const uint32_t interface_number,
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info
) {
  // get handler
  pid_t handler;
  const int result = handler_get( type, &handler );
  // handle error
  if ( result != 0 ) {
    return result;
  }
  // handle success but no handler bound => return success
  if ( -1 == handler ) {
    return 0;
  }
  // set handler pids for device
  device->device_deallocate_handler = handler;
  device->device_detached_handler = handler;
  // generate request
  constexpr size_t request_size = sizeof( vfs_ioctl_perform_request_t ) + sizeof( usb_generic_attach_t );
  vfs_ioctl_perform_request_t* request = malloc( request_size );
  if ( ! request ) {
    // return nomem
    return ENOMEM;
  }
  // clear out
  memset( request, 0, request_size );
  // populate container
  ( ( usb_generic_attach_t* )request->container )->device_number = device_number;
  ( ( usb_generic_attach_t* )request->container)->interface_number = interface_number;
  // attach is defined as first custom message
  const size_t response_id = bolthur_rpc_raise(
    GENERIC_ATTACH,
    handler,
    request,
    request_size,
    callback,
    GENERIC_ATTACH,
    request,
    request_size,
    origin,
    data_info,
    nullptr,
    false
  );
  // handle error
  if ( ! response_id ) {
    free( request );
    return EIO;
  }
  free( request );
  // return success
  return 0;
}
