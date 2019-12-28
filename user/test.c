
#include <stdio.h>
#include <unistd.h>

int i = 0;

int main( int argc, char *argv[] ) {
  printf( "Hello World!\r\n" );

  for ( int j = i + 5; i < j; i++ ) {
    char c = ( char )( ( int )'0' + i );
    printf( "%c", c );
  }

  void *b = sbrk( 0 );
  int *p = ( int* )b;
  sbrk( 2 );

  return 0;
}
