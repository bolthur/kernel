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

#include <paths.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/bolthur.h>
#include "keyboard.h"
#include "keymap.h"
#include "rpc.h"
#include "../../../../libusbd.h"
#include "../../../../../library/usb/usb.h"
#include "../../../../../library/hid/hid.h"
#include "../../../../../library/vfs/dev.h"
#include "../../../../../library/vfs/handler.h"

/**
 * @brief Console file descriptor used for pushing input
 */
int console_fd;

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // register rpc
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Setup rpc handler\r\n" )
  #endif
  if ( !rpc_init() ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to bind rpc handler\r\n" );
    #endif
    return -1;
  }

  // open console
  console_fd = open( _PATH_CONSOLE, O_RDWR );
  if ( -1 == console_fd ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to open console\r\n" )
    #endif
    return -1;
  }

  // initialize usb library
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Setup usb library\r\n" )
  #endif
  int result = usb_init();
  if ( 0 != result ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to bind usb library: %s\r\n", strerror( result ) );
    #endif
    return -1;
  }

  // initialize hid library
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Setup hid library\r\n" )
  #endif
  result = hid_init();
  if ( 0 != result ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to bind usb library: %s\r\n", strerror( result ) );
    #endif
    return -1;
  }

  // query allowed rpc origin
  pid_t allowed_rpc_origin = vfs_get_file_handler( HID_DEVICE_PATH );
  if ( -1 == allowed_rpc_origin ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get handler id of %s\r\n", HID_DEVICE_PATH )
    #endif
    return -1;
  }
  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( allowed_rpc_origin ) ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    #endif
    return -1;
  }
  // query allowed rpc origin
  allowed_rpc_origin = vfs_get_file_handler( USBD_DEVICE_PATH );
  if ( -1 == allowed_rpc_origin ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to get handler id of %s\r\n", USBD_DEVICE_PATH )
    #endif
    return -1;
  }
  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( allowed_rpc_origin ) ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    #endif
    return -1;
  }

  // registering handler
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Registering handler at hid\r\n" )
  #endif
  result = hid_register_handler( LIBUSB_HID_USAGE_PAGE_DESKTOP_KEYBOARD );
  if ( 0 != result ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to register handler at hid\r\n" )
    #endif
    return -1;
  }

  // load keymap
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Loading configured keymap\r\n" )
  #endif
  result = keymap_init();
  if ( 0 != result ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to load keymap\r\n" )
    #endif
    return -1;
  }

  // add device file
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Sending device to vfs\r\n" )
  #endif
  constexpr uint32_t device_info[] = {
    GENERIC_ATTACH,
    GENERIC_DETACH,
    GENERIC_POLL_INTERRUPT,
  };
  if ( ! vfs_dev_add_file( KEYBOARD_DEVICE_PATH, device_info, 3, nullptr ) ) {
    #if defined( KEYBOARD_ENABLE_DEBUG )
      STARTUP_PRINT( "Unable to add dev usbd\r\n" )
    #endif
    return -1;
  }

  // enable rpc
  #if defined( KEYBOARD_ENABLE_DEBUG )
    STARTUP_PRINT( "Enable rpc\r\n" )
  #endif
  _syscall_rpc_set_ready( true );

  // wait for rpc
  bolthur_rpc_wait_block();
  return 0;
}
