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

#ifndef _LIB_TAR_H
#define _LIB_TAR_H

#include <stdbool.h>
#include <stddef.h>

#define TAR_HEADER_SIZE 512

typedef enum {
  TAR_FILE_TYPE_NORMAL_FILE = '0',
  TAR_FILE_TYPE_NORMAL_FILE_EX = '\0',
  TAR_FILE_TYPE_HARD_LINK = '1',
  TAR_FILE_TYPE_SYMBOLIC_LINK = '2',
  TAR_FILE_TYPE_CHARACTER_DEVICE = '3',
  TAR_FILE_TYPE_BLOCK_DEVICE = '4',
  TAR_FILE_TYPE_DIRECTORY = '5',
  TAR_FILE_TYPE_NAMED_PIPE = '6',
} tar_file_type_t;

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
}tar_header_t;

size_t tar_total_size( uintptr_t );
size_t tar_size( tar_header_t* );
tar_header_t* tar_next( tar_header_t* );
tar_header_t* tar_lookup_file( uintptr_t, const char* );
uint8_t* tar_file( tar_header_t* );
bool tar_end_reached( tar_header_t* );
size_t octal_size_to_int( const char*, size_t );

#endif
