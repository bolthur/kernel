
/*#include <stdio.h>
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
}*/

#include <stdint.h>
#include <stddef.h>

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

int pid = 0;
int tid = 0;
char str_pid[ 100 ] = "pid: ";
char str_tid[ 100 ] = "tid: ";
char *digits = "0123456789abcdef";

void _start( void ) {
  // push pid to str_pid
  convert_number( &str_pid[ 5 ], ( uintmax_t )pid, 10, digits );
  convert_number( &str_tid[ 5 ], ( uintmax_t )tid, 10, digits );
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #11 \n\
    mov r0, %[r1] \n\
    svc #11" : : [ r0 ]"r"( str_pid ), [ r1 ]"r"( str_tid )
  );

  // gather pid and tid
  __asm__ __volatile__( "\n\
    svc #3 \n\
    mov %0, r0 \n\
    svc #6 \n\
    mov %1, r0"
    : "=r" ( pid ), "=r" ( tid )
    : : "cc"
  );

  // push pid to str_pid
  convert_number( &str_pid[ 5 ], ( uintmax_t )pid, 10, digits );
  convert_number( &str_tid[ 5 ], ( uintmax_t )tid, 10, digits );
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #11 \n\
    mov r0, %[r1] \n\
    svc #11" : : [ r0 ]"r"( str_pid ), [ r1 ]"r"( str_tid )
  );

  while( 1 ) {
    __asm__ __volatile__( "nop" );
  }
}
