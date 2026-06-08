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
#include "../libusbd.h"

/**
 * @fn int context_attach_create(rpc_handler_t, pid_t, size_t, size_t, const void*, size_t, bool, libusb_device_t*, usbd_attach_context_t**)
 * @brief Helper to allocate context
 * @param callback
 * @param origin
 * @param data_info
 * @param response_info
 * @param original_request
 * @param original_request_size
 * @param address
 * @param device_number
 * @param dev
 * @param with_return
 * @param ctx
 * @return
 */
int usbd_context_attach_create(
  const rpc_handler_t callback,
  const pid_t origin,
  const size_t data_info,
  const size_t response_info,
  const void* original_request,
  const size_t original_request_size,
  const uint8_t address,
  const uint8_t device_number,
  libusb_device_t* dev,
  const bool with_return,
  usbd_attach_context_t** ctx
) {
  // allocate additional context
  *ctx = malloc( sizeof( usbd_attach_context_t ) );
  // handle error
  if ( ! *ctx ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate space for context\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out context
  memset( *ctx, 0, sizeof( usbd_attach_context_t ) );
  // duplicate request
  void* req = malloc( original_request_size );
  if ( ! req ) {
    // free context
    usbd_context_attach_destroy( *ctx );
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate space for context\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // copy over request
  memcpy( req, original_request, original_request_size );
  // populate context
  ( *ctx )->data_info = data_info;
  ( *ctx )->origin = origin;
  ( *ctx )->original_response_info = response_info;
  ( *ctx )->request = req;
  ( *ctx )->request_size = original_request_size;
  ( *ctx )->handler = callback;
  ( *ctx )->device_number = device_number;
  ( *ctx )->device = dev;
  ( *ctx )->address = address;
  ( *ctx )->with_return = with_return;
  // return success
  return 0;
}

/**
 * @fn void context_attach_destroy(usbd_attach_context_t*)
 * @brief Helper to destroy created context
 * @param ctx
 */
void usbd_context_attach_destroy( usbd_attach_context_t* ctx ) {
  if ( ! ctx ) {
    return;
  }
  EARLY_STARTUP_PRINT( "Destroy attach context %p\r\n", ( void* )ctx )
  if ( ctx->request ) {
    free( ctx->request );
  }
  EARLY_STARTUP_PRINT( "Destroy attach context %p\r\n", ( void* )ctx )
  free( ctx );
}

/**
 * @fn int usbd_context_descriptor_create(rpc_handler_t, void*, usbd_attach_context_t**)
 * @brief Helper to allocate context
 * @param callback
 * @param context
 * @param with_return
 * @param ctx
 * @return
 */
int usbd_context_descriptor_create(
  const rpc_handler_t callback,
  void* context,
  bool with_return,
  usbd_descriptor_context_t** ctx
) {
  // allocate additional context
  *ctx = malloc( sizeof( usbd_descriptor_context_t ) );
  // handle error
  if ( ! *ctx ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate space for context\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out context
  memset( *ctx, 0, sizeof( usbd_descriptor_context_t ) );
  // populate context
  ( *ctx )->context = context;
  ( *ctx )->handler = callback;
  ( *ctx )->with_return = with_return;
  // return success
  return 0;
}

/**
 * @fn void usbd_context_descriptor_destroy(usbd_descriptor_context_t*)
 * @brief Helper to destroy created context
 * @param ctx
 */
void usbd_context_descriptor_destroy( usbd_descriptor_context_t* ctx ) {
  if ( ! ctx ) {
    return;
  }
  EARLY_STARTUP_PRINT( "Destroy descriptor context %p\r\n", ( void* )ctx )
  free( ctx );
}

/**
 * @fn int usbd_context_address_create(rpc_handler_t, void*, usbd_attach_context_t**)
 * @brief Helper to allocate context
 * @param callback
 * @param context
 * @param address
 * @param with_return
 * @param ctx
 * @return
 */
int usbd_context_address_create(
  const rpc_handler_t callback,
  void* context,
  uint8_t address,
  bool with_return,
  usbd_address_context_t** ctx
) {
  // allocate additional context
  *ctx = malloc( sizeof( usbd_address_context_t ) );
  // handle error
  if ( ! *ctx ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate space for context\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out context
  memset( *ctx, 0, sizeof( usbd_address_context_t ) );
  // populate context
  ( *ctx )->context = context;
  ( *ctx )->handler = callback;
  ( *ctx )->address = address;
  ( *ctx )->with_return = with_return;
  // return success
  return 0;
}

/**
 * @fn void usbd_context_descriptor_destroy(usbd_address_context_t*)
 * @brief Helper to destroy created context
 * @param ctx
 */
void usbd_context_address_destroy( usbd_address_context_t* ctx ) {
  if ( ! ctx ) {
    return;
  }
  EARLY_STARTUP_PRINT( "Destroy address context %p\r\n", ( void* )ctx )
  free( ctx );
}

/**
 * @fn int usbd_context_configure_create(rpc_handler_t, void*, usbd_configure_context_t**)
 * @brief Helper to allocate context
 * @param callback
 * @param context
 * @param configuration
 * @param with_return
 * @param ctx
 * @return
 */
int usbd_context_configure_create(
  const rpc_handler_t callback,
  void* context,
  uint8_t configuration,
  bool with_return,
  usbd_configure_context_t** ctx
) {
  // allocate additional context
  *ctx = malloc( sizeof( usbd_configure_context_t ) );
  // handle error
  if ( ! *ctx ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate space for context\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out context
  memset( *ctx, 0, sizeof( usbd_configure_context_t ) );
  // populate context
  ( *ctx )->context = context;
  ( *ctx )->handler = callback;
  ( *ctx )->configuration = configuration;
  ( *ctx )->with_return = with_return;
  // return success
  return 0;
}

/**
 * @fn void usbd_context_configure_destroy(usbd_configure_context_t*)
 * @brief Helper to destroy created context
 * @param ctx
 */
void usbd_context_configure_destroy( usbd_configure_context_t* ctx ) {
  if ( ! ctx ) {
    return;
  }
  EARLY_STARTUP_PRINT( "Destroy configure context %p\r\n", ( void* )ctx )
  free( ctx );
}

/**
 * @fn int usbd_context_configuration_create(rpc_handler_t, void*, usbd_configuration_context_t**)
 * @brief Helper to allocate context
 * @param callback
 * @param context
 * @param configuration
 * @param with_return
 * @param ctx
 * @return
 */
int usbd_context_configuration_create(
  const rpc_handler_t callback,
  void* context,
  uint8_t configuration,
  bool with_return,
  usbd_configuration_context_t** ctx
) {
  // allocate additional context
  *ctx = malloc( sizeof( usbd_configuration_context_t ) );
  // handle error
  if ( ! *ctx ) {
    // debug output
    #if defined( USBD_ENABLE_DEBUG )
      EARLY_STARTUP_PRINT( "Unable to allocate space for context\r\n" )
    #endif
    // return error
    return ENOMEM;
  }
  // clear out context
  memset( *ctx, 0, sizeof( usbd_configuration_context_t ) );
  // populate context
  ( *ctx )->context = context;
  ( *ctx )->handler = callback;
  ( *ctx )->configuration = configuration;
  ( *ctx )->with_return = with_return;
  // return success
  return 0;
}

/**
 * @fn void usbd_context_configure_destroy(usbd_configuration_context_t*)
 * @brief Helper to destroy created context
 * @param ctx
 */
void usbd_context_configuration_destroy( usbd_configuration_context_t* ctx ) {
  if ( ! ctx ) {
    return;
  }
  EARLY_STARTUP_PRINT( "Destroy configuration context %p\r\n", ( void* )ctx )
  free( ctx );
}
