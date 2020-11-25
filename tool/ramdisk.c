
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <string.h>
#include <dirent.h>
#include <errno.h>

int main( int argc, char* argv[] ) {
  if ( argc != 3 ) {
    printf( "Usage: ramdisk <driver> <platform>\r\n");
    return 1;
  }
  // acquire platforms
  char* driver = argv[ 1 ];
  char* platform = argv[ 2 ];

  // check against supported ones
  if ( 0 != strcmp( platform, "rpi" ) ) {
    printf( "Supported platforms: rpi\r\n" );
    return 1;
  }

  // try to open directory
  DIR* dir = opendir( driver );
  // handle error
  if ( ! dir ) {
    printf( "Cannot open directory %s\r\n", driver );
    return 1;
  // handle not existing
  } else if ( ENOENT == errno ) {
    printf( "Directory %s does not exist\r\n", driver );
    return 1;
  }

  // FIXME: scan recursive for elf files
  // FIXME: create tmp directory
  // FIXME: copy elf files into tmp/boot
  // FIXME: build tar image
  // FIXME: remove tmp image again


  return closedir( dir );
}
