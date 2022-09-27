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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../ramdisk.h"
#include "../init.h"
#include "../util.h"
#include "../global.h"

#include <stdnoreturn.h>

/**
 * @fn void init_stage1(void)
 * @brief Final init stage starting servers from storage with finally starting shell
 */
noreturn void init_stage3( void ) {
  /// FIXME: Kill unnecessary ramdisk server again
  /// FIXME: Start authentication manager
  /// FIXME: Start USB driver with all attached devices
  /// FIXME: Start login console

  EARLY_STARTUP_PRINT( "size_t max = %zu\r\n", SIZE_MAX )
  EARLY_STARTUP_PRINT( "unsigned long long max = %llu\r\n", ULLONG_MAX )

  EARLY_STARTUP_PRINT( "Adjust stdout / stderr buffering\r\n" )
  // adjust buffering of stdout and stderr
  setvbuf( stdout, NULL, _IOLBF, 0 );
  setvbuf( stderr, NULL, _IONBF, 0 );

  EARLY_STARTUP_PRINT( "äöüÄÖÜ\r\n" )
  int a = printf( "äöüÄÖÜ\r\n" );
  EARLY_STARTUP_PRINT( "äöüÄÖÜ\r\n" )
  //fflush( stdout );
  int b = printf( "Tab test: \"\t\" should be 4 spaces here!\r\n" );
  //fflush( stdout );
  int c = printf( "Testing newline without cr\nFoobar");
  //fflush( stdout );
  int d = printf( ", now with cr\r\nasdf\r\näöüÄÖÜ\r\n" );
  //fflush( stdout );
/*
  pid_t forked_process = fork();
  if ( errno ) {
    EARLY_STARTUP_PRINT( "Unable to fork process: %s\r\n", strerror( errno ) );
    exit( -1 );
  }
  // fork only
  if ( 0 == forked_process ) {
    while ( true ) {
      printf( "what the fork?\r\n" );
      sleep( 2 );
    }
  }*/
  for ( int i = 0; i < 70; i++ ) {
    printf( "stdout: init - %d\r\n", i );
  }

  int e = printf( "stdout: init=>console=>terminal=>framebuffer" );
  fflush( stdout );
  int f = fprintf(
    stderr,
    "stderr: init=>console=>terminal=>framebuffer"
  );
  fflush( stderr );

  EARLY_STARTUP_PRINT( "a = %d, b = %d, c = %d, d = %d, e = %d, f = %d\r\n",
    a, b, c, d, e, f )

  EARLY_STARTUP_PRINT( "Just looping around with nops :O\r\n" )
  while( true ) {
    __asm__ __volatile__( "nop" );
  }

  // exit program!
  EARLY_STARTUP_PRINT( "Init done!\r\n" );
  exit( 0 );
}
