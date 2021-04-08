/**
 * Copyright (C) 2018 - 2021 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

int main( int argc, char *argv[] ) {
  int* p = ( int* )malloc( sizeof( int ) * 4 );
  memset( p, 0, sizeof( int ) * 4 );
  const int address_size = ( int )( sizeof( uintptr_t ) * 2 );
  printf(
    "allocated address = %#0*"PRIxPTR"\r\n",
    address_size, ( uintptr_t )p
  );
  for(;;);
  return 0;
}
