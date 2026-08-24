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
#include <wchar.h>
#if defined( USBD_ENABLE_OUTPUT )
  #include <inttypes.h>
#endif
#include "../libusbd.h"
// library includes
#include  "../../../../library/util/min.h"

/**
 * @fn void string_get_finished( size_t, pid_t, size_t, size_t )
 * @brief get string finished callback
 * @param type rpc type
 * @param origin origin rpc
 * @param data_info data id
 * @param response_info response id
 */
static void string_get_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read string finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get contexts
  const usbd_get_descriptor_context_t* descriptor = async_data->context;
  const usbd_get_string_context_t* ctx = descriptor->context;
  // invoke callback
  ctx->callback( type, origin, data_info, response_info );
  // free request
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn int usbd_string_get(libusb_device_t*, uint8_t, uint16_t, void*, size_t, usbd_read_lang_context_t*, rpc_handler_t)
 * @brief Get usb string
 * @param dev device to get string for
 * @param string_index string index
 * @param lang_id language id
 * @param buffer buffer address
 * @param buffer_length buffer length
 * @param ctx get string context
 * @param callback callback to continue with
 * @return
 */
int usbd_string_get(
  const libusb_device_t* dev,
  const uint8_t string_index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length,
  usbd_read_lang_context_t* ctx,
  const rpc_handler_t callback
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "get string\r\n" )
  #endif
  // allocate get string context
  usbd_get_string_context_t* get_string_context;
  const int result = usbd_context_get_string_create( callback,
    buffer, buffer_length, ctx, &get_string_context );
  // handle error
  if ( 0 != result ) {
    return result;
  }
  // call get async
  return usbd_descriptor_get_async(
    ( libusb_device_t* )dev,
    LIBUSB_DESCRIPTOR_STRING,
    string_index,
    lang_id,
    buffer,
    buffer_length,
    0,
    string_get_finished,
    ctx->context->origin,
    ctx->context->data_info,
    ctx->context->request,
    ctx->context->request_size,
    get_string_context,
    buffer_length
  );
}

/**
 * @fn void string_read_lang_read_finished( size_t, pid_t, size_t, size_t )
 * @brief Read language finished callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void string_read_lang_read_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read language text finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // get contexts
  const usbd_get_descriptor_context_t* descriptor = async_data->context;
  usbd_get_string_context_t* ctx = descriptor->context;
  usbd_read_lang_context_t* read_lang_context = ctx->context;
  // handle error
  if ( read_lang_context->context->device->error ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Error while reading language length" )
    #endif
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    usbd_context_read_string_destroy( read_lang_context->context );
    bolthur_rpc_destroy_async( bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info ) );
    return;
  }
  // call callback
  read_lang_context->callback( type, origin, data_info, response_info );
  usbd_context_get_string_destroy( ctx );
  usbd_context_read_lang_destroy( read_lang_context );
}

/**
 * @fn void string_read_lang_length_finished( size_t, pid_t, size_t, size_t )
 * @brief Read language length finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void string_read_lang_length_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read language length finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // get contexts
  const usbd_get_descriptor_context_t* descriptor = async_data->context;
  usbd_get_string_context_t* ctx = descriptor->context;
  usbd_read_lang_context_t* read_lang_context = ctx->context;
  // handle error
  if ( read_lang_context->context->device->error ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Error while reading language length" )
    #endif
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    usbd_context_read_string_destroy( read_lang_context->context );
    bolthur_rpc_destroy_async( bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info ) );
    return;
  }
  // handle not enough read
  if ( read_lang_context->context->device->last_transfer == read_lang_context->buffer_length ) {
    read_lang_context->callback( type, origin, data_info, response_info );
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    bolthur_rpc_destroy_async( async_data );
    return;
  }
  // destroy context
  usbd_context_get_string_destroy( ctx );
  // continue with string get
  const int result = usbd_string_get(
    read_lang_context->context->device,
    read_lang_context->string_index,
    read_lang_context->language_id,
    read_lang_context->buffer,
    size_min( ( ( uint8_t* )read_lang_context->buffer )[ 0 ], read_lang_context->buffer_length ),
    read_lang_context,
    string_read_lang_read_finished
  );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to fetch string lang\r\n" )
    #endif
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    usbd_context_read_string_destroy( read_lang_context->context );
    bolthur_rpc_destroy_async( bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info ) );
    return;
  }
  bolthur_rpc_destroy_async( async_data );
  bolthur_rpc_destroy_async( bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info ) );
}

/**
 * @fn int usbd_string_read_lang(libusb_device_t*, uint8_t, uint16_t, void*, size_t, usbd_read_string_context_t*, rpc_handler_t)
 * @brief Get usb string lang
 * @param dev device to get string for
 * @param string_index string index
 * @param lang_id language id
 * @param buffer buffer address
 * @param buffer_length buffer length
 * @param ctx read string context
 * @param callback callback to continue with
 * @return
 */
int usbd_string_read_lang(
  const libusb_device_t* dev,
  const uint8_t string_index,
  const uint16_t lang_id,
  void* buffer,
  const size_t buffer_length,
  usbd_read_string_context_t* ctx,
  const rpc_handler_t callback
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read language\r\n" )
  #endif
  // create context
  usbd_read_lang_context_t* read_lang_context;
  const int result = usbd_context_read_lang_create(
    buffer,
    buffer_length,
    string_index,
    lang_id,
    callback,
    ctx,
    &read_lang_context
  );
  // handle error
  if ( 0 != result ) {
    return result;
  }
  // get string length
  return usbd_string_get(
    dev,
    string_index,
    lang_id,
    buffer,
    size_min( 2, buffer_length ),
    read_lang_context,
    string_read_lang_length_finished
  );
}

/**
 * @fn void string_read_language_data_finished( size_t, pid_t, size_t, size_t )
 * @brief Callback for read language finished
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
static void string_read_language_data_finished(
  size_t type,
  pid_t origin,
  size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read string language data finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_peek_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // get contexts
  const usbd_get_descriptor_context_t* descriptor_context = async_data->context;
  usbd_get_string_context_t* ctx = descriptor_context->context;
  usbd_read_lang_context_t* read_lang_context = ctx->context;
  const usbd_read_string_context_t *read_string_context = read_lang_context->context;
  // transform buffer
  auto const descriptor = ( libusb_string_descriptor_t* )read_string_context->buffer;
  // cache descriptor length
  const uint8_t descriptor_length = ( uint8_t )( ( descriptor->descriptor_length - 2 ) >> 1 );
  // allocate temp string
  const size_t temp_length = sizeof( uint8_t ) * descriptor_length;
  uint8_t* temp = malloc( temp_length );
  if ( ! temp ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate temp area for string\r\n" )
    #endif
    // dummy error response
    // return
    vfs_ioctl_perform_response_t err_response = { .status = -ENOMEM };
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    return;
  }
  // translate data into buffer
  uint8_t i;
  uint8_t data_index = 0;
  for ( i = 0; i < descriptor_length; i++ ) {
    temp[ i ] = ( uint8_t )wctob( descriptor->data[ data_index++ ] );
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "i = %"PRIu8" / %"PRIu8" / %"PRIu16" / %c\r\n", i, temp[ i ], descriptor->data[ data_index - 1 ], temp[ i ] )
    #endif
  }
  memcpy( read_string_context->buffer, temp, temp_length );
  // add null termination
  if ( i < read_string_context->buffer_length ) {
    ( ( uint8_t* )read_string_context->buffer)[ i ] = '\0';
  }
  free( temp );
  // return
  read_string_context->handler( type, origin, data_info, response_info );
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn void string_read_language_id_finished( size_t, pid_t, size_t, size_t )
 * @brief Read language id finished
 * @param type rpc type
 * @param origin origin process
 * @param data_info data id
 * @param response_info response id
 */
static void string_read_language_id_finished(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] size_t data_info,
  size_t response_info
) {
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read string language id finished\r\n" )
  #endif
  // peek matching async data without destroy for call chain
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async(
    RPC_VFS_IOCTL, response_info );
  // handle no async data
  if ( ! async_data ) {
    // cleanup
    _syscall_rpc_cleanup();
    // skip rest
    return;
  }
  // dummy error response
  vfs_ioctl_perform_response_t err_response = { .status = -EINVAL };
  // get contexts
  const usbd_get_descriptor_context_t* descriptor = async_data->context;
  usbd_get_string_context_t* ctx = descriptor->context;
  usbd_read_lang_context_t* read_lang_context = ctx->context;
  usbd_read_string_context_t* read_string_context = read_lang_context->context;
  // handle error
  if ( read_string_context->device->error || read_string_context->device->last_transfer < 4 ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Error while reading language length" )
    #endif
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    usbd_context_read_string_destroy( read_string_context );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  // read whole string
  const int result = usbd_string_read_lang(
    read_string_context->device,
    read_string_context->string_index,
    read_string_context->language_data[ 1 ],
    read_string_context->buffer,
    read_string_context->buffer_length,
    read_string_context,
    string_read_language_data_finished
  );
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to read language\r\n" )
    #endif
    usbd_context_get_string_destroy( ctx );
    usbd_context_read_lang_destroy( read_lang_context );
    usbd_context_read_string_destroy( read_string_context );
    // return
    err_response.status = -EPROTO;
    bolthur_rpc_return( RPC_VFS_IOCTL, &err_response, sizeof( err_response ), async_data, 0 );
    return;
  }
  bolthur_rpc_destroy_async( async_data );
}

/**
 * @fn int usbd_string_read(libusb_device_t*, uint8_t, void*, size_t, void*, size_t, rpc_handler_t, pid_t, size_t)
 * @brief Read usb string
 * @param dev device to get string for
 * @param string_index string index
 * @param buffer buffer address
 * @param buffer_length buffer length
 * @param request original request
 * @param request_size original request size
 * @param handler handler to be called
 * @param origin origin process
 * @param data_info data id
 * @return
 */
int usbd_string_read(
  libusb_device_t* dev,
  const uint8_t string_index,
  void* buffer,
  const size_t buffer_length,
  void* request,
  const size_t request_size,
  const rpc_handler_t handler,
  const pid_t origin,
  const size_t data_info
) {
  // validate parameter
  if ( ! buffer || ! string_index || ! buffer_length ) {
    return EINVAL;
  }
  // debug output
  #if defined( USBD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Read string\r\n" )
  #endif
  // create context
  usbd_read_string_context_t* ctx;
  int result = usbd_context_read_string_create(
    handler, dev, string_index, 0, buffer, buffer_length, request, request_size,
    origin, data_info, &ctx);
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to create get string context: %s\r\n", strerror( result ) )
    #endif
    // return result
    return result;
  }
  // read language ids
  result = usbd_string_read_lang(
    dev,
    0,
    0,
    ctx->language_data,
    sizeof( ctx->language_data ),
    ctx,
    string_read_language_id_finished
  );
  // handle error
  if ( 0 != result ) {
    // debug output
    #if defined( USBD_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to fetch language ids\r\n" )
    #endif
    // return result
    return result;
  }
  // return success
  return 0;
}
