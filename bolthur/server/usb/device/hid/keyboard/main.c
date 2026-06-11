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
  STARTUP_PRINT( "Setup rpc handler\r\n" )
  if ( !rpc_init() ) {
    STARTUP_PRINT( "Unable to bind rpc handler\r\n" );
    return -1;
  }

  // open console
  console_fd = open( _PATH_CONSOLE, O_RDWR );
  if ( -1 == console_fd ) {
    STARTUP_PRINT( "Unable to open console\r\n" )
    return -1;
  }

  // initialize usb library
  STARTUP_PRINT( "Setup usb library\r\n" )
  int result = usb_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to bind usb library: %s\r\n", strerror( result ) );
    return -1;
  }

  // initialize hid library
  STARTUP_PRINT( "Setup hid library\r\n" )
  result = hid_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to bind usb library: %s\r\n", strerror( result ) );
    return -1;
  }

  // query allowed rpc origin
  const pid_t allowed_rpc_origin = vfs_get_file_handler( HID_DEVICE_PATH );
  if ( -1 == allowed_rpc_origin ) {
    STARTUP_PRINT( "Unable to get handler id of %s\r\n", HID_DEVICE_PATH )
    return -1;
  }

  // push to valid origin
  if ( ! bolthur_rpc_origin_push_valid( allowed_rpc_origin ) ) {
    STARTUP_PRINT( "Unable to push mount pid to valid origin list!\r\n" )
    return -1;
  }

  // registering handler
  STARTUP_PRINT( "Registering handler at hid\r\n" )
  result = hid_register_handler( LIBUSB_HID_USAGE_PAGE_DESKTOP_KEYBOARD );
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to register handler at hid\r\n" )
    return -1;
  }

  // load keymap
  STARTUP_PRINT( "Loading configured keymap\r\n" )
  result = keymap_init();
  if ( 0 != result ) {
    STARTUP_PRINT( "Unable to load keymap\r\n" )
    return -1;
  }

  // add device file
  STARTUP_PRINT( "Sending device to vfs\r\n" )
  constexpr uint32_t device_info[] = {
    GENERIC_ATTACH,
    GENERIC_DETACH,
    GENERIC_DEALLOCATE,
  };
  if ( ! vfs_dev_add_file( KEYBOARD_DEVICE_PATH, device_info, 3, nullptr ) ) {
    STARTUP_PRINT( "Unable to add dev usbd\r\n" )
    return -1;
  }

  // enable rpc
  STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  // debug message
  STARTUP_PRINT( "Starting polling loop\r\n" )
  // get clock frequency
  const double frequency = _syscall_timer_frequency();
  // endless loop to start polling and finally wait for rpc
  while ( true ) {
    // start with head
    libusb_keyboard_device_t* current = keyboard_head;
    // variable for min sleep time
    long sleep_time = 0;
    // loop while there is something
    while ( current != NULL ) {
      // handle already polling
      if ( 0 != current->running_poll ) {
        // go to next
        current = current->next;
        // skip rest
        continue;
      }
      // handle already polling
      if ( 0 != current->last_poll ) {
        // get expected sleep time in seconds
        const long expected_sleep_time = current->descriptor.interval;
        // get current timer tick count
        const size_t tick_count = _syscall_timer_tick_count();
        // calculate real sleep time
        const long real_sleep_time = (long)(expected_sleep_time -
          (((double)tick_count - (double)current->last_poll) / frequency) * 1000);
        #if defined( KEYBOARD_ENABLE_DEBUG )
          EARLY_STARTUP_PRINT( "real_sleep_time %ld\r\n", real_sleep_time )
        #endif
        // handle sleep
        if (
          real_sleep_time > 0
          && (
            0 == sleep_time
            || sleep_time > real_sleep_time
          )
        ) {
          // set sleep time
          sleep_time = real_sleep_time;
        }
        // handle sleep
        if ( real_sleep_time > 0 ) {
          // go to next
          current = current->next;
          // skip rest
          continue;
        }
      }
      #if defined( KEYBOARD_ENABLE_DEBUG )
        EARLY_STARTUP_PRINT( "START POLLING\r\n" )
      #endif
      // start keyboard polling
      if ( 0 == keyboard_start_polling( current ) ) {
        // set last poll to tick count
        current->last_poll = _syscall_timer_tick_count();
        // set proper sleep time when it's 0 or greater interval
        if (
          0 == sleep_time
          || sleep_time > current->descriptor.interval
        ) {
          sleep_time = current->descriptor.interval;
        }
      }
      // go to next
      current = current->next;
    }
    // handle waiting for rpc
    if (0 == sleep_time) {
      sleep_time = 1000;
    }
    EARLY_STARTUP_PRINT( "sleep_time = %ld\r\n", sleep_time )
    // sleep till next poll
    nanosleep( &(struct timespec){
      .tv_sec = sleep_time / 1000,
      .tv_nsec = ( sleep_time % 1000 ) * 1000000,
    }, NULL );
  }
}
