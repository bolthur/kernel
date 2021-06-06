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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "list.h"

#ifndef __IMAGE_H__
#define __IMAGE_H__

// entry point type
typedef int ( *main_entry_point )( int, char** );
// some forward declarations
struct elf_image;
typedef struct elf_image elf_image_t;
typedef struct elf_image* elf_image_ptr_t;

void image_list_cleanup_helper( const list_item_ptr_t );
void image_destroy_data_entry( elf_image_ptr_t );

uint32_t image_name_hash( const char* );
bool image_validate_machine( void* );
bool image_validate( void*, int );
uint8_t* image_buffer_file( const char*, size_t* );
bool image_list_data_create( list_manager_ptr_t, void*, size_t, int, char* );
bool image_list_contain( list_manager_ptr_t, char* );
bool image_load( void*, size_t );
bool image_handle_dynamic( elf_image_ptr_t, list_manager_ptr_t );
bool image_handle_flat( elf_image_ptr_t );
void* image_get_section_by_type( void*, size_t );

#endif
