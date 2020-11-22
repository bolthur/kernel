
// FIXME: TEST WITH MULTIPLE THREADS!

char str[] = "\r\nexit thread\r\n";

void _start( void ) {
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str )
  );
  // just exit :)
  __asm__ __volatile__( "svc #12" ::: "cc" );
}
