/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include <time.h>
#include <libtar.h>
#include "../ramdisk.h"

/**
 * @brief Append a tar file with given mode
 * @param t
 * @param realname
 * @param savename
 * @param mode
 * @return
 */
bool ramdisk_append_tar_file_with_mode(
  TAR* t,
  const char* realname,
  const char* savename,
  const mode_t mode
) {
  struct stat st;
  if ( -1 == stat( realname, &st ) ) {
    fprintf( stderr, "%s: stat() failed\n", realname );
    return false;
  }
  // populate header with stat from real path
  th_set_from_stat( t, &st );
  // set path
  th_set_path( t, savename );
  // overwrite mode
  th_set_mode( t, mode );
  // overwrite user and group
  th_set_user( t, 0 );
  th_set_group( t, 0 );
  // set mtime
  th_set_mtime( t, time( nullptr ) );
  // finish it before writing
  th_finish( t );
  // write to tar
  if ( th_write( t ) ) {
    fprintf( stderr, "%s: th_write() failed\n", realname );
    return false;
  }
  // append content in case it's a regular file
  if ( TH_ISREG( t ) && tar_append_regfile( t, realname ) ) {
    fprintf( stderr, "%s: th_append_regfile() failed\n", realname );
    return false;
  }
  // return success
  return true;
}
