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
#include <sys/errno.h>
#include <sys/bolthur.h>
#include <sys/unistd.h>

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  // print something
  STARTUP_PRINT( "login processing!\r\n" )
  // endlessly looping login shell
  while ( true ) {
    char* username = malloc( sizeof( char ) * 1024 );
    if ( ! username ) {
      STARTUP_PRINT( "Unable to allocate memory for username!\r\n" )
      return -1;
    }

    // read username
    printf( "username: " );
    fflush( stdout );
    if ( ! fgets( username, 1024, stdin ) ) {
      STARTUP_PRINT( "Unable to read username: %s!\r\n", strerror( errno ) )
      free( username );
      continue;
    }
    // append end of string
    username[ strlen( username ) - 1 ] = '\0';
    // read password
    char* password = getpass( "password: " );

    // debug output
    STARTUP_PRINT( "username: %s\r\npassword: %s\r\n", username, password );
    // debug endless loop
    while ( true ) {
      __asm__ __volatile__ ( "nop" );
    }

    // FIXME: check user and password
    // FIXME: When user and password are not matching, free username and password and continue loop
    // FIXME: fork process and start shell from /etc/passwd and prepare environment variables from /etc/passwd

    free( username );
  }
  // exit with success
  return 0;
}
