
#include <stdlib.h>
#include <string.h>

int main( int argc, char *argv[] ) {
  int* p = ( int* )malloc( sizeof( int ) * 4 );
  memset( p, 0, sizeof( int ) * 4 );
  return 0;
}
