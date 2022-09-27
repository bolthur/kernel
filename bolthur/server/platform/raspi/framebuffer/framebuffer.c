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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/bolthur.h>
#include "framebuffer.h"
#include "../../../../library/collection/list/list.h"
#include "../libiomem.h"
#include "../../../libframebuffer.h"

static size_t memory_counter = 1;
static list_manager_t* memory_list = NULL;

uint32_t physical_width;
uint32_t physical_height;
uint32_t virtual_width;
uint32_t virtual_height;
uint32_t pitch;
uint32_t size;
uint32_t total_size;
uint8_t* screen;
uint8_t* current_back;
uint8_t* front_buffer;
uint8_t* back_buffer;

int iomem_fd;

framebuffer_rpc_t command_list[] = {
  {
    .command = FRAMEBUFFER_GET_RESOLUTION,
    .callback = framebuffer_handle_resolution
  }, {
    .command = FRAMEBUFFER_CLEAR,
    .callback = framebuffer_handle_clear
  }, {
    .command = FRAMEBUFFER_FLIP,
    .callback = framebuffer_handle_flip
  }, {
    .command = FRAMEBUFFER_SURFACE_RENDER,
    .callback = framebuffer_handle_surface_render
  }, {
    .command = FRAMEBUFFER_SURFACE_ALLOCATE,
    .callback = framebuffer_handle_surface_allocate
  },
};


/**
 * @fn int32_t memory_lookup(const list_item_t*, const void*)
 * @brief List lookup helper
 *
 * @param a
 * @param data
 * @return
 */
static int32_t memory_lookup(
  const list_item_t* a,
  const void* data
) {
  return ( ( framebuffer_memory_t* )a->data )->id == ( size_t )data
    ? 0 : 1;
}

/**
 * @fn void memory_cleanup(list_item_t*)
 * @brief List cleanup helper
 *
 * @param a
 */
static void memory_cleanup( list_item_t* a ) {
  // convert
  framebuffer_memory_t* mem = a->data;
  // handle shared memory id
  if ( mem->shm_id ) {
    // detach shared memory
    while ( true ) {
      _syscall_memory_shared_detach( mem->shm_id );
      if ( errno ) {
        continue;
      }
      break;
    }
  }
  // default cleanup
  list_default_cleanup( a );
}

/**
 * @fn bool framebuffer_init(void)
 * @brief Init framebuffer
 *
 * @return
 *
 * @todo create shared area with similar size than framebuffer
 */
bool framebuffer_init( void ) {
  // create list
  memory_list = list_construct( memory_lookup, memory_cleanup, NULL );
  if ( ! memory_list ) {
    return false;
  }
  // open device
  iomem_fd = open( IOMEM_DEVICE_PATH, O_RDWR );
  if ( -1 == iomem_fd ) {
    list_destruct( memory_list );
    return false;
  }
  // build request
  size_t request_size = sizeof( int32_t ) * 8;
  int32_t* request = malloc( request_size );
  if ( ! request ) {
    list_destruct( memory_list );
    close( iomem_fd );
    return false;
  }
  memset( request, 0, request_size );
  // populate request
  request[ 0 ] = ( int32_t )request_size; // buffer size
  request[ 1 ] = 0; // perform request
  request[ 2 ] = 0x40003; // display size request
  request[ 3 ] = 8; // buffer size
  request[ 4 ] = 0; // request size
  request[ 5 ] = 0; // space for horizontal resolution
  request[ 6 ] = 0; // space for vertical resolution
  request[ 7 ] = 0; // end tag
  // perform request
  int result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  if ( -1 == result ) {
    list_destruct( memory_list );
    free( request );
    close( iomem_fd );
    return false;
  }
  // set physical width and height from response
  physical_width = ( uint32_t )request[ 5 ];
  physical_height = ( uint32_t )request[ 6 ];
  free( request );
  // Use fallback if not set
  if ( 0 == physical_width && 0 == physical_height ) {
    physical_width = FRAMEBUFFER_SCREEN_WIDTH;
    physical_height = FRAMEBUFFER_SCREEN_HEIGHT;
  }
  EARLY_STARTUP_PRINT(
    "Using resolution %ldx%ld\r\n",
    physical_width,
    physical_height
  )

  // build request
  request_size = sizeof( int32_t ) * 35;
  request = malloc( request_size );
  if ( ! request ) {
    list_destruct( memory_list );
    close( iomem_fd );
    return false;
  }
  memset( request, 0, request_size );
  // populate request
  size_t idx = 0;
  request[ idx++ ] = ( int32_t )request_size; // buffer size
  request[ idx++ ] = 0; // request
  // set physical size
  request[ idx++ ] = 0x00048003;
  request[ idx++ ] = 8; // value buffer size (bytes)
  request[ idx++ ] = 8; // request + value length (bytes)
  request[ idx++ ] = ( int32_t )physical_width; // horizontal resolution
  request[ idx++ ] = ( int32_t )physical_height; // vertical resolution
  // set virtual size
  request[ idx++ ] = 0x00048004;
  request[ idx++ ] = 8; // value buffer size (bytes)
  request[ idx++ ] = 8; // request + value length (bytes)
  // FIXME: width and height multiplied by 2 for qemu framebuffer flipping
  request[ idx++ ] = ( int32_t )physical_width; // * 2; // horizontal resolution
  request[ idx++ ] = ( int32_t )physical_height * 2; // vertical resolution
  // set depth
  request[ idx++ ] = 0x00048005;
  request[ idx++ ] = 4; // value buffer size (bytes)
  request[ idx++ ] = 4; // request + value length (bytes)
  request[ idx++ ] = FRAMEBUFFER_SCREEN_DEPTH; // 32 bpp
  // set virtual offset
  request[ idx++ ] = 0x00048009;
  request[ idx++ ] = 8; // value buffer size (bytes)
  request[ idx++ ] = 8; // request + value length (bytes)
  request[ idx++ ] = 0; // space for x
  request[ idx++ ] = 0; // space for y
  // get pitch
  request[ idx++ ] = 0x00040008;
  request[ idx++ ] = 4; // buffer size
  request[ idx++ ] = 0; // request size
  request[ idx++ ] = 0; // space for pitch
  // set pixel order
  request[ idx++ ] = 0x00048006;
  request[ idx++ ] = 4; // value buffer size (bytes)
  request[ idx++ ] = 4; // request + value length (bytes)
  request[ idx++ ] = 0; // RGB encoding /// FIXME: CORRECT VALUE HERE?
  // allocate framebuffer
  request[ idx++ ] = 0x00040001;
  request[ idx++ ] = 8; // value buffer size (bytes)
  request[ idx++ ] = 4; // request + value length (bytes)
  request[ idx++ ] = 0x1000; // alignment = 0x1000
  request[ idx++ ] = 0; // space for response
  request[ idx++ ] = 0; // end tag
  // perform request
  result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MAILBOX,
      request_size,
      IOCTL_RDWR
    ),
    request
  );
  if ( -1 == result ) {
    list_destruct( memory_list );
    free( request );
    close( iomem_fd );
    return false;
  }
  // save screen information
  physical_width = ( uint32_t )request[ 5 ];
  physical_height = ( uint32_t )request[ 6 ];
  virtual_width = ( uint32_t )request[ 10 ];
  virtual_height = ( uint32_t )request[ 11 ];
  // save screen address as current and buffer size
  screen = ( uint8_t* )request[ 32 ];
  total_size = ( uint32_t )request[ 33 ];
  // save pitch
  pitch = ( uint32_t )request[ 24 ];
  // set correct sizes
  size = pitch * physical_height;
  // free request again
  free( request );

  // mask screen address ( necessary when caches are not enabled )
  uintptr_t mask = 0xc0000000;
  uintptr_t uscreen = ( uintptr_t )screen;
  uscreen &= ~mask;
  EARLY_STARTUP_PRINT( "Screen address from mailbox: %p\r\n", ( void* )screen )
  EARLY_STARTUP_PRINT( "Screen address masked: %p\r\n", ( void* )uscreen )
  // write back
  screen = ( uint8_t* )uscreen;

  // try to map framebuffer area and handle possible error
  void* tmp = mmap(
    ( void* )screen,
    total_size,
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PHYSICAL | MAP_DEVICE,
    -1,
    0
  );
  if( MAP_FAILED == tmp ) {
    list_destruct( memory_list );
    close( iomem_fd );
    return false;
  }
  EARLY_STARTUP_PRINT( "Screen address returned by mmap: %p\r\n", tmp )
  // overwrite screen
  screen = ( uint8_t* )tmp;
  // clear everything after init completely
  memset( screen, 0, total_size );
  // set front and back buffers
  front_buffer = screen;
  back_buffer = ( uint8_t* )( screen + size );
  current_back = back_buffer;
  // return with register of necessary rpc
  return framebuffer_register_rpc();
}

/**
 * @fn bool framebuffer_register_rpc(void)
 * @brief Register necessary rpc handler
 *
 * @return
 */
bool framebuffer_register_rpc( void ) {
  // register all handlers
  size_t max = sizeof( command_list ) / sizeof( command_list[ 0 ] );
  // loop through handler to identify used one
  for ( size_t i = 0; i < max; i++ ) {
    // register rpc
    bolthur_rpc_bind( command_list[ i ].command, command_list[ i ].callback, true );
    if ( errno ) {
      return false;
    }
  }
  return true;
}

/**
 * @fn void framebuffer_flip(void)
 * @brief Framebuffer flip to back buffer
 */
void framebuffer_flip( void ) {
  // default request for screen flip
  static int32_t buffer[] = { 0, 0, 0x00048009, 0x8, 0, 0, 0 };
  // handle switch
  if ( screen == front_buffer ) {
    buffer[ 6 ] = ( int32_t )physical_height;
    screen = back_buffer;
    current_back = front_buffer;
  } else {
    buffer[ 6 ] = 0;
    screen = front_buffer;
    current_back = back_buffer;
  }
  // perform request
  int result = ioctl(
    iomem_fd,
    IOCTL_BUILD_REQUEST(
      IOMEM_RPC_MAILBOX,
      sizeof( int32_t ) * 7,
      IOCTL_RDWR
    ),
    buffer
  );
  if ( -1 == result ) {
    return;
  }
  // copy over screen into back buffer
  memcpy( current_back, screen, size );
}

/**
 * @fn void framebuffer_handle_resolution(size_t, pid_t, size_t, size_t)
 * @brief Handle resolution request currently only get
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 *
 * @todo return shared memory id
 */
void framebuffer_handle_resolution(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate response
  vfs_ioctl_perform_response_t* response;
  size_t response_size = sizeof( *response )
    + sizeof( framebuffer_resolution_t );
  response = malloc( response_size );
  // handle error
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // clear out response
  memset( response, 0, response_size );
  // local variable for resolution data
  framebuffer_resolution_t resolution_data = {
    .width = physical_width,
    .height = physical_height,
    .depth = FRAMEBUFFER_SCREEN_DEPTH,
  };
  // copy over data
  memcpy( response->container, &resolution_data, sizeof( framebuffer_resolution_t ) );
  // return from rpc
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  // free response
  free( response );
}

/**
 * @fn void framebuffer_handle_clear(size_t, pid_t, size_t, size_t)
 * @brief Handle clear request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_clear(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // clear
  memset( current_back, 0, size );
  // perform flip
  framebuffer_flip();
  // set success and return
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
}

/**
 * @fn void framebuffer_handle_flip(size_t, pid_t, size_t, size_t)
 * @brief Handle flip request
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_flip(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // remove data
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  framebuffer_flip();
  // return success
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
}

/**
 * @fn void framebuffer_handle_surface_render(size_t, pid_t, size_t, size_t)
 * @brief RPC callback for rendering a surface
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_surface_render(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate  structure
  framebuffer_surface_render_t* info = malloc( sizeof( *info ) );
  if ( ! info ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data( info, sizeof( *info ), data_info, false );
  // handle error
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }
  // get item
  list_item_t* item = list_lookup_data( memory_list, ( void* )info->surface_id );
  if ( ! item ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }
  // get memory item
  framebuffer_memory_t* mem = item->data;
  memcpy( current_back, mem->address, size );
  /*
  // determine max width, height and pitch
  uint32_t max_width = mem->width > physical_width
    ? physical_width : mem->width;
  uint32_t max_height = mem->height > physical_height
    ? physical_height : mem->height;
  uint32_t bpp = mem->depth / CHAR_BIT;
  // render surface
  for ( uint32_t idx = 0; idx < mem->height * mem->pitch; idx += bpp ) {
    uint32_t y = idx / mem->width;
    uint32_t x = idx % mem->height;
    if (
      x >= max_width
      || y >= max_height
    ) {
      continue;
    }
    // calculate x and y values for rendering
    uint32_t final_x = info->x + ( x / bpp );
    uint32_t final_y = info->y + y;
    // extract color from data
    uint32_t color = *( ( uint32_t* )( &mem->address[ y * mem->pitch + x  ] ) );
    / *EARLY_STARTUP_PRINT(
      "x = %"PRId32", y = %"PRId32", final_x = %"PRId32", final_y = %"PRId32", color = %"PRIx32"\r\n",
      x / bpp, y, final_x, final_y, color )* /
    // determine render offset
    uint32_t offset = final_y * pitch + final_x * BYTE_PER_PIXEL;
    // push back color
    *( ( uint32_t* )( current_back + offset ) ) = color;
  }*/
  // free again
  free( info );
  // return success
  error.status = 0;
  bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
}

/**
 * @fn void framebuffer_handle_surface_allocate(size_t, pid_t, size_t, size_t)
 * @brief RPC callback for allocate a surface
 *
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void framebuffer_handle_surface_allocate(
  __unused size_t type,
  pid_t origin,
  size_t data_info,
  __unused size_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // handle no data
  if( ! data_info ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // allocate space for data
  framebuffer_surface_allocate_t* info = malloc( sizeof( *info ) );
  if ( ! info ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    return;
  }
  // fetch rpc data
  _syscall_rpc_get_data(
    info, sizeof( *info ), data_info, false );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }

  // calculated line length
  uint32_t allocate_pitch = info->width * ( info->depth / CHAR_BIT );
  size_t memory_size = allocate_pitch * info->height;
  // request shared memory
  size_t shm_id = _syscall_memory_shared_create( memory_size );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }
  // attach shared area
  void* shm_addr = _syscall_memory_shared_attach( shm_id, 0 );
  if ( errno ) {
    error.status = -errno;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }
  // clearout shared memory
  memset( shm_addr, 0, memory_size );
  // allocate memory for management structure
  framebuffer_memory_t* mem = malloc( sizeof( *mem ) );
  if ( ! mem ) {
    while ( true ) {
      _syscall_memory_shared_detach( shm_id );
      if ( errno ) {
        continue;
      }
      break;
    }
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }
  // clearout and fill
  memset( mem, 0, sizeof( *mem ) );
  mem->width = info->width;
  mem->height = info->height;
  mem->pitch = allocate_pitch;
  mem->address = shm_addr;
  mem->shm_id = shm_id;
  mem->id = memory_counter++;
  mem->depth = info->depth;
  // push back
  if ( ! list_push_back_data( memory_list, mem ) ) {
    while ( true ) {
      _syscall_memory_shared_detach( shm_id );
      if ( errno ) {
        continue;
      }
      break;
    }
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    return;
  }

  // fill response into structure
  info->shm_id = shm_id;
  info->surface_id = mem->id;
  info->pitch = allocate_pitch;

  // allocate response
  vfs_ioctl_perform_response_t* response;
  size_t response_size = sizeof( *response )
    + sizeof( framebuffer_surface_allocate_t );
  response = malloc( response_size );
  // handle error
  if ( ! response ) {
    while ( true ) {
      _syscall_memory_shared_detach( shm_id );
      if ( errno ) {
        continue;
      }
      break;
    }
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), NULL );
    free( info );
    free( mem );
    return;
  }
  // clear out response and copy data
  memset( response, 0, response_size );
  memcpy( response->container, info, sizeof( framebuffer_surface_allocate_t ) );
  // return from rpc and free remaining stuff
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, NULL );
  free( response );
  free( info );
}
