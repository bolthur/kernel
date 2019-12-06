
int i = 0;

void _start( void ) {
  for ( int j = i + 5; i < j; i++ ) {
    char c = ( char )( ( int )'0' + i );

    __asm__ __volatile__( "\n\
      mov r0, %[r0] \n\
      svc #10" : : [ r0 ]"r"( c )
    );
  }

  while( 1 ) {}
}
