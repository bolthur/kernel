
#include <stdio.h>

int i = 0;

int main( int argc, char *argv[] ) {
  for ( int j = i + 5; i < j; i++ ) {
    char c = ( char )( ( int )'0' + i );
    putchar( c );
  }

  return 0;
}
