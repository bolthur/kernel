
// FIXME: TEST WITH MULTIPLE THREADS!

void _start( void ) {
  // just exit :)
  __asm__ __volatile__( "svc #2" ::: "cc" );
}
