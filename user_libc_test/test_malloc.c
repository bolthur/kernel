
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

int main( int argc, char *argv[] ) {
  int* p = ( int* )malloc( sizeof( int ) * 4 );
  memset( p, 0, sizeof( int ) * 4 );
  const int address_size = ( int )( sizeof( uintptr_t ) * 2 );
  printf(
    "allocated address = %#0*"PRIxPTR"\r\n",
    address_size, ( uintptr_t )p
  );
  fflush( stdout );
  return 0;
}
