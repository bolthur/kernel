
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if ! defined( __LIB_TAR__ )
#define __LIB_TAR__

typedef struct {
  char file_name[ 100 ];
  char file_mode[ 8 ];
  char user_id[ 8 ];
  char group_id[ 8 ];
  char file_size[ 12 ];
  char last_modified[ 12 ];
  char checksum[ 8 ];
  char file_type;
  char linked_file_name[ 100 ];
  char __padding[ 255 ];
} tar_header_t, *tar_header_ptr_t;

uint64_t tar_get_total_size( uintptr_t );

#endif
