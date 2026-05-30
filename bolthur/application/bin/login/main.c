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

#include <fcntl.h>
#include <stdio.h>
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include "../../../server/libauthentication.h"

/**
 * @brief Child pid
 */
pid_t child = 0;

/**
 * @fn static char* build_env(const char*, const char*)
 * @brief Helper to build env
 * @param value environment value
 * @param prefix prefix
 * @return
 */
static char* build_env( const char* value, const char* prefix ) {
  const size_t env_len = strlen( value ) + strlen( prefix ) + 1;
  char* home_env = malloc( env_len );
  if ( ! home_env ) {
    return NULL;
  }
  memset( home_env, 0, env_len );
  sprintf( home_env, "%s=%s", prefix, value );
  return home_env;
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  /// FIXME: IMPLEMENT gethostname
  printf( "%s login!\r\n", "bolthur" );
  fflush( stdout );
  // endlessly looping login shell
  while ( true ) {
    // allocate space for username
    char* username = malloc( sizeof( char ) * 1024 );
    // handle error
    if ( ! username ) {
      STARTUP_PRINT( "Unable to allocate memory for username!\r\n" )
      return -1;
    }
    // read username
    printf( "username: " );
    fflush( stdout );
    if ( ! fgets( username, 1024, stdin ) ) {
      printf( "Login failed\r\n" );
      fflush( stdout );
      free( username );
      continue;
    }
    // strip out newline from username
    if ( strlen( username ) ) {
      username[ strlen( username ) - 1 ] = '\0';
    }
    // drain out newline from stdin
    char drain[2];
    fgets( drain, 2, stdin );
    // read password
    const char* password = getpass( "password: " );
    // open authentication device
    const int fd = open( AUTHENTICATION_DEVICE, O_RDWR );
    if ( -1 == fd ) {
      printf( "Login failed\r\n" );
      fflush( stdout );
      free( username );
      continue;
    }
    // allocate shared memory
    const size_t shm_id = _syscall_memory_shared_create( sizeof( authentication_request_request_data_t ) );
    if ( errno ) {
      printf( "Login failed\r\n" );
      fflush( stdout );
      free( username );
      close( fd );
      continue;
    }
    // attach shared memory
    authentication_request_request_data_t *request_data;
    do {
      request_data = _syscall_memory_shared_attach( shm_id, ( uintptr_t )NULL );
      if ( errno ) {
        continue;
      }
      break;
    } while( true );
    // allocate request
    authentication_request_request_t* request = malloc( sizeof( authentication_request_request_t ) );
    if ( ! request ) {
      printf( "Login failed\r\n" );
      fflush( stdout );
      free( username );
      close( fd );
      continue;
    }
    // clear out
    memset( request, 0, sizeof( authentication_request_request_t ) );
    // copy over stuff
    strncpy( request_data->user, username, 1023 );
    strncpy( request_data->password, password, 127 );
    request->process = getpid();
    request->shm_id = shm_id;
    // perform request
    const int result = ioctl(
      fd,
      IOCTL_BUILD_REQUEST(
        AUTHENTICATE_REQUEST,
        sizeof( authentication_request_request_t ),
        IOCTL_RDWR
      ),
      request
    );
    // handle error
    if ( -1 == result ) {
      const int e = errno;
      printf( "Login failed: %s\r\n", strerror( e ) );
      fflush( stdout );
      free( username );
      free( request );
      close( fd );
      _syscall_memory_shared_detach( shm_id );
      continue;
    }
    // fork process
    child = fork();
    // handle error
    if ( 0 > child ) {
      const int e = errno;
      printf( "Login failed: %s\r\n", strerror( e ) );
      fflush( stdout );
      free( username );
      free( request );
      close( fd );
      _syscall_memory_shared_detach( shm_id );
      continue;
    }
    // child only
    if ( 0 == child ) {
      char* base = basename( request_data->pw_shell );
      if ( ! base ) {
        exit( 1 );
      }
      // build command
      char* cmd[] = { base, NULL, };
      // build home env
      char* home_env = build_env( request_data->pw_home, "HOME=" );
      if ( ! home_env ) {
        exit( 1 );
      }
      // build shell env
      char* shell_env = build_env( request_data->pw_shell, "SHELL=" );
      if ( ! shell_env ) {
        exit( 1 );
      }
      char* env[] = { home_env, shell_env, NULL };
      // exec to replace
      if ( -1 == execve( request_data->pw_shell, cmd, env ) ) {
        exit( 1 );
      }
    }

    // free resources
    free( username );
    free( request );
    close( fd );
    _syscall_memory_shared_detach( shm_id );
    // wait for rpc while child is existing
    while ( child ) {
      _syscall_rpc_wait_for_call();
    }
  }
  // exit success
  return 0;
}
