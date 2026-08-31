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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include <sys/unistd.h>
#include "../configuration.h"
#include "../init.h"
#include "../global.h"

/**
 * @fn void init_stage1(void)
 * @brief Final init stage starting servers from storage with finally starting shell
 */
void init_stage3( void ) {
  // start servers by configuration
  if ( ! configuration_handle( "/ramdisk/config/stage3.ini", nullptr ) ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Something went wrong with stage3 startup!\r\n" )
    #endif
    exit( 1 );
  }
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Starting login process\r\n" )
  #endif
/*  // close device manager since everythig was fired up
  close( fd_dev_manager );
  // fork for starting the login shell
  pid_t forked = fork();
  if ( forked == 0 ) {
    // build command
    char* cmd[] = { "login", nullptr, };
    // exec to replace
    if ( -1 == execv( "/bin/login", cmd ) ) {
      EARLY_STARTUP_PRINT( "Error during exec, exiting: %s\r\n", strerror( errno ) )
      exit( 1 );
    }
  }
  if ( 0 > forked ) {
    EARLY_STARTUP_PRINT( "Error while forking: %s\r\n", strerror( -forked ) )
  }*/
}
