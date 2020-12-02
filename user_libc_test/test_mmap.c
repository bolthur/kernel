
#include <sys/types.h>
#include <sys/mman.h>
#include <stddef.h>

__attribute__((weak))
void _start( void ) {
  void *addr = NULL;
  register unsigned r0 __asm__( "r0" ) = ( unsigned )NULL;
  register unsigned r1 __asm__( "r1" ) = sizeof( uint32_t ) * 4;
  register unsigned r2 __asm__( "r2" ) = PROT_READ | PROT_WRITE;
  register unsigned r3 __asm__( "r3" ) = MAP_PRIVATE;
  register signed r4 __asm__( "r4" ) = -1;
  register unsigned r5 __asm__( "r5" ) = 0;
  __asm__ __volatile__( "\n\
      svc #21"
        : "+r"( r0 ) // addr
        : "r"( r1 ), // len
          "r"( r2 ), // prot
          "r"( r3 ), // flags
          "r"( r4 ), // file descriptor
          "r"( r5 ) // offset
        : "cc"
    );
  /*
   *
  int result;
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    mov r1, %[r1] \n\
    svc #22 \n\
    mov %[r2], r0"
      : [ r2 ]"=r"( result )
      : [ r0 ]"r"( addr ),
        [ r1 ]"r"( len )
      : "cc"
  );
  return result;
   */
  for( ;; ) {}
}
