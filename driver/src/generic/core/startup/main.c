/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/bolthur.h>

int main( __unused int argc, __unused char* argv[] ) {
  // print something
  EARLY_STARTUP_PRINT( "startup processing!\r\n" )
  char buf[ 5 ];

  memset( buf, 0, 5 );
  int fd = open( "/ramdisk/test/hello.txt", 0 );
  EARLY_STARTUP_PRINT( "startup: _open( %s, 0, 0 ) = %d\r\n", "/ramdisk/test/hello.txt", fd )
  EARLY_STARTUP_PRINT( "startup: read( %d, %#"PRIxPTR", %d ) = %d\r\n", fd, ( uintptr_t )buf, 3, read( fd, ( void* )buf, 3 ) )
  EARLY_STARTUP_PRINT( "startup: buf = \"%s\"\r\n", buf )
  EARLY_STARTUP_PRINT( "startup: close( %d ) = %d\r\n", fd, close( fd ) )
  memset( buf, 0, 5 );
  int fd2 = open( "/ramdisk/test/world.txt", O_CREAT, 1 );
  EARLY_STARTUP_PRINT( "startup: _open( %s, 0, 0 ) = %d\r\n", "/ramdisk/test/world.txt", fd2 )
  EARLY_STARTUP_PRINT( "startup: read( %d, %#"PRIxPTR", %d ) = %d\r\n", fd, ( uintptr_t )buf, 3, read( fd2, ( void* )buf, 3 ) )
  EARLY_STARTUP_PRINT( "startup: buf = \"%s\"\r\n", buf )
  EARLY_STARTUP_PRINT( "startup: _close( %d ) = %d\r\n", fd2, close( fd2 ) )
  int tmp = close(fd2);
  EARLY_STARTUP_PRINT( "startup: close( %d ) = %d => %s\r\n", fd2, tmp, strerror( errno ) )
  memset( buf, 0, 5 );
  int fd3 = open( "/ramdisk/test/multiline.txt", 0 );
  EARLY_STARTUP_PRINT( "startup: _open( %s, 0, 0 ) = %d\r\n", "/ramdisk/test/multiline.txt", fd3 )
  EARLY_STARTUP_PRINT( "startup: read( %d, %#"PRIxPTR", %d ) = %d\r\n", fd, ( uintptr_t )buf, 3, read( fd3, ( void* )buf, 3 ) )
  EARLY_STARTUP_PRINT( "startup: buf = \"%s\"\r\n", buf )
  EARLY_STARTUP_PRINT( "startup: close( %d ) = %d\r\n", fd3, close( fd3 ) )

  for(;;);
  return 0;
}
