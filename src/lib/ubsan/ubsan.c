
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
#include <ubsan.h>
#include <stdlib.h>
#include <core/panic.h>

#define is_aligned( value, alignment ) !( value & ( alignment - 1 ) )

const char* type_check_kind[] = {
  "load of",
  "store to",
  "reference binding to",
  "member access within",
  "member call on",
  "constructor call on",
  "downcast of",
  "downcast of",
  "upcast of",
  "cast to virtual base of",
};

/**
 * @brief Internal helper to print location
 *
 * @param location
 */
static void print( ubsan_source_location_ptr_t location ) {
  printf( "\tfile: %s\r\n\tline: %d\r\n\tcolumn: %d\r\n",
    location->file, location->line, location->column );
}

/**
 * @brief Helper with generic type mismatch handling
 *
 * @param mismatch
 * @param pointer
 */
static void handle_type_mismatch_generic(
  ubsan_type_mismatch_data_generic_ptr_t mismatch,
  uintptr_t pointer
) {
  // null pointer access
  if ( 0 == pointer ) {
    printf( "Null pointer access\r\n" );
  // unaligned access
  } else if (
    0 != mismatch->alignment
    && is_aligned( pointer, mismatch->alignment )
  ) {
    printf( "Unaligned memory access\r\n" );
  // handle size mismatch
  } else {
      printf(
        "Insufficient size\r\n%s address %p with insufficient space for object of type %s\n",
        type_check_kind[ mismatch->type_check_kind ], ( void* )pointer,
        mismatch->type->name );
  }
  // print location where it happened
  print( mismatch->location );
}

/**
 * @brief Type mismatch handling v1
 *
 * @param data
 * @param ptr
 */
void __no_return __ubsan_handle_type_mismatch_v1(
  ubsan_type_mismatch_data_v1_ptr_t data,
  uintptr_t ptr
) {
  // build structure
  ubsan_type_mismatch_data_generic_t generic_data = {
    .location = &data->location,
    .type = data->type,
    .alignment = data->alignment,
    .type_check_kind = data->type_check_kind,
  };
  // handle mismatch
  handle_type_mismatch_generic( &generic_data, ptr );
  // abort further execution
  abort();
}

/**
 * @brief Type mismatch handling
 *
 * @param data
 * @param ptr
 */
void __no_return __ubsan_handle_type_mismatch(
  ubsan_type_mismatch_data_ptr_t data,
  uintptr_t ptr
) {
  // build structure
  ubsan_type_mismatch_data_generic_t generic_data = {
    .location = &data->location,
    .type = data->type,
    .alignment = data->alignment,
    .type_check_kind = data->type_check_kind,
  };
  // handle mismatch
  handle_type_mismatch_generic( &generic_data, ptr );
  // abort further execution
  abort();
}

/**
 * @brief Pointer overflow handling
 *
 * @param data
 * @param before
 * @param after
 */
void __no_return __ubsan_handle_pointer_overflow(
  ubsan_pointer_overflow_data_ptr_t data,
  uint64_t before,
  uint64_t after
) {
  printf( "pointer overflow!\r\nbefore: %lu, after: %lu\r\n", before, after );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief Add overflow handling
 *
 * @param data
 * @param left
 * @param right
 */
void __no_return __ubsan_handle_add_overflow(
  __maybe_unused ubsan_overflow_data_ptr_t data,
  __maybe_unused uint64_t left,
  __maybe_unused uint64_t right
) {
  printf(
    "add overflow!\r\ntype: %s, value: %lu, value: %lu\r\n",
    data->type->name, left, right );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief Subtract overflow handling
 *
 * @param data
 * @param left
 * @param right
 */
void __no_return __ubsan_handle_sub_overflow(
  ubsan_overflow_data_ptr_t data,
  uint64_t left,
  uint64_t right
) {
  printf(
    "sub overflow!\r\ntype: %s, value: %lu, value: %lu\r\n",
    data->type->name, left, right );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief Multiplication overflow handling
 *
 * @param data
 * @param left
 * @param right
 */
void __no_return __ubsan_handle_mul_overflow(
  __maybe_unused ubsan_overflow_data_ptr_t data,
  __maybe_unused uint64_t left,
  __maybe_unused uint64_t right
) {
  printf(
    "mul overflow!\r\ntype: %s, value: %lu, value: %lu\r\n",
    data->type->name, left, right );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief
 *
 * @param data
 * @param left
 * @param right
 */
void __no_return __ubsan_handle_divrem_overflow(
  ubsan_overflow_data_ptr_t data,
  uint64_t left,
  uint64_t right
) {
  printf(
    "divrem overflow!\r\ntype: %s, value: %lu, value: %lu\r\n",
    data->type->name, left, right );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief Out of bounds handling
 *
 * @param data
 * @param left
 * @param right
 */
void __no_return __ubsan_handle_shift_out_of_bounds(
  ubsan_shift_out_of_bounds_data_ptr_t data,
  uint64_t left,
  uint64_t right
) {
  printf(
    "Shift out of bounds!\r\nleft: %s, value: %lu, right: %s, value: %lu\r\n",
    data->left->name, left, data->right->name, right );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief Out of bounds handling
 *
 * @param data
 * @param index
 */
void __no_return __ubsan_handle_out_of_bounds(
  ubsan_out_of_bounds_data_ptr_t data,
  uint64_t index
) {
  printf( "Out of bounds!\r\narray_type: %s, index: %lu\r\n",
    data->array->name, index );
  // print location
  print( &data->location );
  // abort execution
  abort();
}

/**
 * @brief Load invalid value handling
 *
 * @param data
 * @param value
 */
void __no_return __ubsan_handle_load_invalid_value(
  __maybe_unused ubsan_invalid_value_data_ptr_t data,
  __maybe_unused uint64_t value
) {
  printf( "Load invalid value!\r\narray_type: %s, index: %lu\r\n",
    data->type->name, value );
  // print location
  print( &data->location );
  // abort execution
  abort();
}
