
#include <sys/types.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char *argv[] ) {
  mmap( NULL, 0x10000, 3, 3, -1, 0x18 );
}
