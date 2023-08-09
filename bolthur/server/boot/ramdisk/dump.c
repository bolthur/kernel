/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include <tar.h>
#include <sys/bolthur.h>
#include "../ramdisk.h"

/**
 * @fn void ramdisk_dump(TAR*)
 * @brief Dump opened tar
 *
 * @param t
 */
void ramdisk_dump( TAR* t ) {
  // variables
  ramdisk_read_offset = 0;
  // loop through ramdisk and lookup file
  while ( th_read( t ) == 0 ) {
    if ( TH_ISREG( t ) ) {
      // get filename
      char* filename = th_get_pathname( t );
      EARLY_STARTUP_PRINT( "%10s - %s\r\n", "file", filename )
      // skip to next file
      if ( tar_skip_regfile( t ) != 0 ) {
        EARLY_STARTUP_PRINT( "tar_skip_regfile(): %s\n", strerror( errno ) )
        break;
      }
    } else if ( TH_ISSYM( t ) ) {
      EARLY_STARTUP_PRINT( "%10s - %s -> %s\r\n", "symlink", th_get_pathname( t ), th_get_linkname( t ) )
    }
  }
}
