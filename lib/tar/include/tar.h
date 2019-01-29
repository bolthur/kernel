
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#ifndef __LIBTAR__
#define __LIBTAR__

#include <stdint.h>

/**
 * @see https://wiki.osdev.org/Tar
 * @see https://en.wikipedia.org/wiki/Tar_(computing)
 * @see https://www.gnu.org/software/tar/manual/html_node/Standard.html
 */

#define TAR_MAGIC "ustar"
#define TAR_MAGIC_LENGTH 6

#define TAR_VERSION "00"
#define TAR_VERSION_LENGTH 2

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct {
  char file_name[ 100 ];
  char file_mode[ 8 ];
  char owner_user_id[ 8 ];
  char owner_group_id[ 8 ];
  char file_size[ 12 ];
  char last_modification_time[ 12 ];
  char header_checksum[ 8 ];
  char linked_indicator;
  char linked_file_name[ 100 ];
  char ustar_magic[ 6 ];
  char ustar_version[ 2 ];
  char owner_user_name[ 32 ];
  char owner_user_group[ 32 ];
  char device_major_number[ 8 ];
  char device_minor_number[ 8 ];
  char filename_prefix[ 155 ];
} tar_header_t;

uint32_t tar_get_size( const uint8_t* );

#if defined( __cplusplus )
}
#endif

#endif
