
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

char str[ 100 ] = "alloc: ";
char str_success[ 100 ] = "release succeeded";
char str_fail[ 100 ] = "release failed";
char *digits = "0123456789abcdef";

void test_bad_alloc( void ) {
  uintptr_t alloc;
  // Allocate some stuff invalid
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    mov r1, %[r1] \n\
    svc #21 \n\
    mov %[r2], r0"
      : [ r2 ]"=r"( alloc )
      : [ r0 ]"r"( 0x1234 ), [ r1 ]"r"( 5 )
      : "cc"
  );
  // append address
  convert_number( &str[ 7 ], ( uintmax_t )alloc, 16, digits );
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str ) : "cc"
  );
}

void test_good_alloc1( void ) {
  uintptr_t alloc;
  // Allocate some stuff
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    mov r1, %[r1] \n\
    svc #21 \n\
    mov %[r2], r0"
      : [ r2 ]"=r"( alloc )
      : [ r0 ]"r"( 0x1234 ), [ r1 ]"r"( 0 )
      : "cc"
  );
  // append address
  convert_number( &str[ 7 ], ( uintmax_t )alloc, 16, digits );
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str ) : "cc"
  );
}
void test_good_alloc2( void ) {
  uintptr_t alloc;
  // Allocate some stuff
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    mov r1, %[r1] \n\
    svc #21 \n\
    mov %[r2], r0"
      : [ r2 ]"=r"( alloc )
      : [ r0 ]"r"( 0x1234 ), [ r1 ]"r"( 1 )
      : "cc"
  );
  // append address
  convert_number( &str[ 7 ], ( uintmax_t )alloc, 16, digits );
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str ) : "cc"
  );
}

void _start( void ) {
  uintptr_t alloc;
  uint32_t count = 2;
  bool result;

  test_bad_alloc();
  test_good_alloc1();
  test_good_alloc2();

  // Allocate some stuff
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    mov r1, %[r1] \n\
    svc #21 \n\
    mov %[r2], r0"
      : [ r2 ]"=r"( alloc )
      : [ r0 ]"r"( 0x1234 ), [ r1 ]"r"( 2 )
      : "cc"
  );
  // append address
  convert_number( &str[ 7 ], ( uintmax_t )alloc, 16, digits );
  // call for put string
  __asm__ __volatile__( "\n\
    mov r0, %[r0] \n\
    svc #102" : : [ r0 ]"r"( str ) : "cc"
  );

  // Deallocate stuff
  __asm__ __volatile__( "\n\
    mov r0, %[address] \n\
    mov r1, %[count] \n\
    svc #22 \n\
    mov %[result], r0"
      : [ result ]"=r"( result )
      : [ address ]"r"( alloc ), [ count ]"r"( count )
      : "cc"
  );

  // call for put string
  if ( result ) {
    __asm__ __volatile__( "\n\
      mov r0, %[r0] \n\
      svc #102" : : [ r0 ]"r"( str_success ) : "cc"
    );
  } else {
    __asm__ __volatile__( "\n\
      mov r0, %[r0] \n\
      svc #102" : : [ r0 ]"r"( str_fail ) : "cc"
    );
  }

  // FIXME: Free page by page again ( 2 in this case )
  // just exit :)
  __asm__ __volatile__( "svc #2" ::: "cc" );
}
