
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

#if ! defined( __LIB_UBSAN__ )
#define __LIB_UBSAN__

#include <stdint.h>

typedef struct {
  const char* file;
  uint32_t line;
  uint32_t column;
} ubsan_source_location_t, *ubsan_source_location_ptr_t;

typedef struct {
  uint16_t kind;
  uint16_t info;
  char name[];
} ubsan_type_descriptor_t, *ubsan_type_descriptor_ptr_t;

typedef struct {
  ubsan_source_location_t location;
  ubsan_type_descriptor_ptr_t array;
  ubsan_type_descriptor_ptr_t index;
} ubsan_out_of_bounds_data_t, *ubsan_out_of_bounds_data_ptr_t;

typedef struct {
  ubsan_source_location_t location;
  ubsan_type_descriptor_ptr_t left;
  ubsan_type_descriptor_ptr_t right;
} ubsan_shift_out_of_bounds_data_t, *ubsan_shift_out_of_bounds_data_ptr_t;

typedef struct {
  ubsan_source_location_t location;
  ubsan_type_descriptor_ptr_t type;
} ubsan_overflow_data_t, *ubsan_overflow_data_ptr_t;

typedef struct {
  ubsan_source_location_t location;
} ubsan_pointer_overflow_data_t, *ubsan_pointer_overflow_data_ptr_t;

typedef struct {
  ubsan_source_location_t location;
  ubsan_type_descriptor_ptr_t type;
  uintptr_t alignment;
  uint8_t type_check_kind;
} ubsan_type_mismatch_data_t, *ubsan_type_mismatch_data_ptr_t;

typedef struct {
  ubsan_source_location_t location;
  ubsan_type_descriptor_ptr_t type;
  const uint8_t alignment;
  const uint8_t type_check_kind;
} ubsan_type_mismatch_data_v1_t, *ubsan_type_mismatch_data_v1_ptr_t;

typedef struct {
  ubsan_source_location_ptr_t location;
  ubsan_type_descriptor_ptr_t type;
  uintptr_t alignment;
  uint8_t type_check_kind;
} ubsan_type_mismatch_data_generic_t, *ubsan_type_mismatch_data_generic_ptr_t;

typedef struct {
  ubsan_source_location_t location;
  ubsan_type_descriptor_ptr_t type;
} ubsan_invalid_value_data_t, *ubsan_invalid_value_data_ptr_t;

void __ubsan_handle_type_mismatch( ubsan_type_mismatch_data_ptr_t, uintptr_t );
void __ubsan_handle_type_mismatch_v1( ubsan_type_mismatch_data_v1_ptr_t, uintptr_t );
void __ubsan_handle_pointer_overflow( ubsan_pointer_overflow_data_ptr_t, uint64_t, uint64_t );
void __ubsan_handle_add_overflow( ubsan_overflow_data_ptr_t, uint64_t, uint64_t );
void __ubsan_handle_sub_overflow( ubsan_overflow_data_ptr_t, uint64_t, uint64_t );
void __ubsan_handle_mul_overflow( ubsan_overflow_data_ptr_t, uint64_t, uint64_t );
void __ubsan_handle_divrem_overflow( ubsan_overflow_data_ptr_t, uint64_t, uint64_t );
void __ubsan_handle_shift_out_of_bounds( ubsan_shift_out_of_bounds_data_ptr_t, uint64_t, uint64_t );
void __ubsan_handle_out_of_bounds( ubsan_out_of_bounds_data_ptr_t, uint64_t );
void __ubsan_handle_load_invalid_value( ubsan_invalid_value_data_ptr_t, uint64_t );
void __ubsan_handle_negate_overflow( ubsan_overflow_data_ptr_t, uint64_t );

#endif
