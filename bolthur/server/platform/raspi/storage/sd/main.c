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
#include <stdint.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include <inttypes.h>
#include "sd.h"
#include "../../../../libhelper.h"

// for testing
#include "../../../../../library/fs/mbr.h"

#include <bosl/error.h>
#include <bosl/scanner.h>
#include <bosl/parser.h>
#include <bosl/interpreter.h>
#include <bosl/environment.h>
#include <bosl/object.h>
#include <bosl/binding.h>

/**
 * @brief Helper to read file content
 *
 * @param path
 * @return
 */
__unused static char* read_file( const char* path ) {
  // open script
  FILE* f = fopen( path, "r" );
  if ( !f ) {
    perror( path );
    return NULL;
  }
  // switch to end, get size and rewind to beginning
  fseek( f, 0, SEEK_END );
  // get position
  long l_size = ftell( f );
  if ( -1L == l_size ) {
    fclose( f );
    return NULL;
  }
  size_t size = ( size_t )l_size + 1;
  // rewind to begin
  rewind( f );
  // allocate memory
  char* buffer = malloc( sizeof( char ) * size );
  if ( !buffer ) {
    fclose( f );
    return NULL;
  }
  // clear out
  memset( buffer, 0, sizeof( char ) * size );
  // read whole file into buffer
  if ( 1 != fread( buffer, size - 1, 1, f ) ) {
    fclose( f );
    free( buffer );
    return NULL;
  }
  // close and return buffer
  fclose( f );
  return buffer;
}

/**
 * @brief Some simple c binding
 */
static bosl_object_t* c_foo(
  __unused bosl_object_t* o,
  list_manager_t* parameter
) {
  // get parameter
  bosl_object_t* parameter_object = bosl_object_extract_parameter( parameter, 0 );
  if ( !parameter_object ) {
    bosl_interpreter_emit_error( NULL, "Unable to extract parameter!" );
    return NULL;
  }
  // ensure type
  if ( BOSL_OBJECT_TYPE_UINT_8 != parameter_object->type ) {
    bosl_interpreter_emit_error( NULL, "Invalid parameter type received!" );
    return NULL;
  }
  // get and copy value ( everything is stored with maximum amount of space )
  uint64_t value = 0;
  memcpy( &value, parameter_object->data, sizeof( value ) );
  // do something
  printf( "c_foo!\r\nparameter1 = %"PRIu64"\r\n", value );
  // return nothing as nothing will be returned
  return NULL;
}

/**
 * @brief Some simple c binding
 */
static bosl_object_t* c_foo_2(
  __unused bosl_object_t* o,
  __unused list_manager_t* parameter
) {
  // do something
  printf( "c_foo_2!\r\n" );
  bosl_object_t* r = bosl_binding_build_return_int( BOSL_OBJECT_TYPE_INT_8, -1 );
  if ( !r ) {
    bosl_interpreter_emit_error( NULL, "Unable to build return in binding!" );
    return NULL;
  }
  // return nothing as nothing will be returned
  return r;
}

/**
 * @brief Some simple c binding
 */
static bosl_object_t* c_foo_3(
  __unused bosl_object_t* o,
  __unused list_manager_t* parameter
) {
  bosl_interpreter_emit_error( NULL, "c_foo_3 error!" );
  return NULL;
}

/**
 * @brief Interpret buffer
 *
 * @param buffer code to interpret
 * @return
 */
__unused static bool interpret( char* buffer ) {
  // initialize object handling
  if ( !bosl_object_init() ) {
    fprintf( stderr, "Unable to init object!\r\n" );
    return false;
  }
  // initialize scanner
  if ( !bosl_scanner_init( buffer ) ) {
    bosl_object_free();
    fprintf( stderr, "Unable to init scanner!\r\n" );
    return false;
  }
  // scan token
  list_manager_t* token_list = bosl_scanner_scan();
  if ( !token_list ) {
    bosl_object_free();
    bosl_scanner_free();
    return false;
  }
  // init parser
  if ( !bosl_parser_init( token_list ) ) {
    bosl_object_free();
    bosl_scanner_free();
    return false;
  }
  // parse ast
  list_manager_t* ast_list = bosl_parser_scan();
  if ( !ast_list ) {
    bosl_object_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }
  // setup bindings
  if ( !bosl_binding_init() ) {
    bosl_object_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }
  if ( !bosl_binding_bind_function( "c_foo", c_foo ) ) {
    bosl_binding_free();
    bosl_object_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }
  if ( !bosl_binding_bind_function( "c_foo_2", c_foo_2 ) ) {
    bosl_binding_free();
    bosl_object_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }
  if ( !bosl_binding_bind_function( "c_foo_3", c_foo_3 ) ) {
    bosl_binding_free();
    bosl_object_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }
  // setup interpreter
  if ( !bosl_interpreter_init( ast_list ) ) {
    bosl_binding_free();
    bosl_object_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }

  // run code
  if ( !bosl_interpreter_run() ) {
    bosl_binding_free();
    bosl_object_free();
    bosl_interpreter_free();
    bosl_parser_free();
    bosl_scanner_free();
    return false;
  }

  // destroy object, parser, scanner and interpreter
  bosl_binding_free();
  bosl_object_free();
  bosl_parser_free();
  bosl_scanner_free();
  bosl_interpreter_free();
  // return success
  return true;
}

/**
 * @fn int main(int, char*[])
 * @brief main entry point
 *
 * @param argc
 * @param argv
 * @return
 */
int main( __unused int argc, __unused char* argv[] ) {
  // read file into buffer
  /*EARLY_STARTUP_PRINT( "Reading file!\r\n" )
  char* buffer = read_file( "/ramdisk/bosl/storage/sd/sdhost/load_data.bosl" );
  if ( !buffer ) {
    EARLY_STARTUP_PRINT( "Reading file failed!\r\n" )
    return -1;
  }
  EARLY_STARTUP_PRINT( "Interpreting script!\r\n" )
  // interpret it
  if ( !interpret( buffer ) ) {
    EARLY_STARTUP_PRINT( "Interpretation failed!\r\n" )
    free( buffer );
    return -1;
  }
  EARLY_STARTUP_PRINT( "continue!\r\n" )
  fflush( stdout );*/
  // allocate space for mbr
  void* buffer = malloc( sizeof( mbr_t ) );
  if ( ! buffer ) {
    EARLY_STARTUP_PRINT( "Unable to allocate space for mbr: %s\r\n", strerror( errno ) )
    return -1;
  }
  EARLY_STARTUP_PRINT( "buffer = %p, end = %p\r\n",
    buffer,
    ( void* )( ( uintptr_t )buffer + sizeof( mbr_t ) )
  )
  // setup emmc
  EARLY_STARTUP_PRINT( "Setup sd interface\r\n" )
  if( ! sd_init() ) {
    EARLY_STARTUP_PRINT(
      "Error while initializing sd interface: %s\r\n",
      sd_last_error()
    )
    return -1;
  }
  // try to read mbr from card
  EARLY_STARTUP_PRINT( "Parsing mbr with partition information\r\n" )
  if ( ! sd_transfer_block(
    ( uint32_t* )buffer,
    sizeof( mbr_t ),
    0,
    SD_OPERATION_READ
  ) ) {
    EARLY_STARTUP_PRINT(
      "Error while reading mbr from card: %s\r\n",
      sd_last_error()
    )
    return -1;
  }
  mbr_t* bpb = buffer;

  // check signature
  EARLY_STARTUP_PRINT( "Check signature\r\n" )
  if (
    bpb->signatur[ 0 ] != 0x55
    && bpb->signatur[ 1 ] != 0xAA
  ) {
    EARLY_STARTUP_PRINT(
      "Invalid signature within mbr: %#"PRIx8", %#"PRIx8"\r\n",
      bpb->signatur[ 0 ],
      bpb->signatur[ 1 ]
    )
    return -1;
  }

  // loop through partitions and print type
  for ( int i = 0; i < 4; i++ ) {
    EARLY_STARTUP_PRINT(
      "partition %d has type %#"PRIx8"\r\n",
      i,
      bpb->partition_table[ i ].data.system_id
    )
  }

  //return -1;

  // enable rpc
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );

  EARLY_STARTUP_PRINT( "Sending device to vfs\r\n" )
  // allocate memory for add request
  vfs_add_request_t* msg = malloc( sizeof( vfs_add_request_t ) );
  if ( ! msg ) {
    return -1;
  }
  // clear memory
  memset( msg, 0, sizeof( vfs_add_request_t ) );
  // prepare message structure
  msg->info.st_mode = S_IFCHR;
  strncpy( msg->file_path, "/dev/sd", PATH_MAX - 1 );
  // perform add request
  send_vfs_add_request( msg, 0, 0 );
  // free again
  free( msg );

  // wait for rpc
  EARLY_STARTUP_PRINT( "Wait for rpc\r\n" )
  bolthur_rpc_wait_block();
  return 0;
}
