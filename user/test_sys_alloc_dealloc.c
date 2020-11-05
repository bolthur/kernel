
#include <stdint.h>
#include <stddef.h>

void _start( void ) {
  // Allocate some stuff
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #21" : : [ r0 ]"r"( 0x1234 )
  );
  // FIXME: Free page by page again ( 2 in this case )
  // just exit :)
  __asm__ __volatile__( "svc #2" ::: "cc" );
}
