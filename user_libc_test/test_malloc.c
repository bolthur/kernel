
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void convert_number(
  char* destination,
  uintmax_t value,
  uintmax_t base,
  const char* digits
) {
  // size to return
  size_t result = 1;
  // value copy
  uintmax_t copy = value;
  // while base smaller than value copy
  while ( base <= copy ) {
    copy /= base;
    result++;
  }
  // write digits
  for ( size_t i = result; i != 0; i-- ) {
    destination[ i - 1 ] = digits[ value % base ];
    value /= base;
  }
  destination[ result++ ] = '\r';
  destination[ result++ ] = '\n';
  destination[ result ] = '\0';
}

int main( int argc, char *argv[] ) {
  int* p = ( int* )malloc( sizeof( int ) * 4 );

  char str[ 32 ] = "";
  char *digits = "0123456789abcdef";

  convert_number( &str[ 0 ], ( uintmax_t )p, 16, digits );
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str )
  );

  memset( p, 0, sizeof( int ) * 4 );

  return 0;
}
