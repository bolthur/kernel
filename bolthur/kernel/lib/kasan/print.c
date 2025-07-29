/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#include "../inttypes.h"
#include "../stdio.h"
#include "kasan.h"
#include "../../debug/debug.h"

/**
 * @fn void kasan_print_16_bytes_no_bug(const char*, uintptr_t)
 * @brief Print 16 bytes no bug
 * @param prefix
 * @param address
 */
void kasan_print_16_bytes_no_bug(const char* prefix, const uintptr_t address) {
  printf( "%s%#"PRIxPTR":", prefix, address );
  for ( uintptr_t i = 0; i < 16; i++ ) {
    printf( " %02"PRIx8, *( uint8_t* )( address + i ) );
  }
  printf( "\r\n" );
}

/**
 * @fn void kasan_print_16_bytes_with_bug(const char*, uintptr_t, uintptr_t)
 * @brief Print 16 bytes with bug
 * @param prefix
 * @param address
 * @param buggy_offset
 */
void kasan_print_16_bytes_with_bug(
  const char* prefix,
  const uintptr_t address,
  const uintptr_t buggy_offset
) {
  printf( "%s%#"PRIxPTR":", prefix, address );
  for ( uintptr_t i = 0; i < buggy_offset; i++ ) {
    printf( " %02"PRIx8, *( uint8_t* )( address + i ) );
  }
  printf( "[%02"PRIx8"]", *( uint8_t* )( address + buggy_offset ) );
  if ( buggy_offset < 15 ) {
    printf( "%02"PRIx8, *( uint8_t* )( address + buggy_offset + 1 ) );
  }
  for ( uintptr_t i = buggy_offset + 2; i < 16; i++ ) {
    printf( " %02"PRIx8, *( uint8_t* )( address + i ) );
  }
  printf( "\r\n" );
}

/**
 * @fn void kasan_print_shadow_memory(uintptr_t, uintptr_t, uintptr_t)
 * @brief Function to print shadow memory
 * @param addr
 * @param before
 * @param after
 */
void kasan_print_shadow_memory(
  const uintptr_t addr,
  uintptr_t before,
  uintptr_t after
) {
  const uintptr_t shadow_address = KASAN_MEM_TO_SHADOW( addr );
  const uintptr_t aligned_shadow = shadow_address & 0xfffffff0;
  const uintptr_t buggy_offset = shadow_address - aligned_shadow;

  printf( "[KASan] Shadow bytes around the buggy address %#"PRIxPTR" (shadow %#"PRIxPTR"):\r\n",
    addr, shadow_address );
  const uintptr_t max_before = shadow_address - kasan_shadow_memory_start;
  const uintptr_t max_after = kasan_shadow_memory_start - 1 - shadow_address;
  if ( before * 16 > max_before ) {
    before = max_before / 16;
  }
  if ( after * 16 > max_after ) {
    after = max_after / 16;
  }
  // print memory before
  for (uintptr_t i = before; i > 0; i--) {
    kasan_print_16_bytes_no_bug("[KASan]   ", aligned_shadow - i * 16);
  }
  // print buggy memory
  kasan_print_16_bytes_with_bug("[KASan] =>", aligned_shadow, buggy_offset);
  // print memory after
  for (uintptr_t i = 1; i <= after; i++) {
    kasan_print_16_bytes_no_bug("[KASan]   ", aligned_shadow + i * 16);
  }
}

/**
 * @fn void kasan_bug_report(uintptr_t, size_t, uintptr_t, bool, uintptr_t)
 * @brief Wrapper to print found bug
 * @param addr
 * @param size
 * @param buggy_shadow_address
 * @param write
 * @param pc
 */
void kasan_bug_report(
  const uintptr_t addr,
  const size_t size,
  const uintptr_t buggy_shadow_address,
  const bool write,
  const uintptr_t pc
) {
  [[maybe_unused]] uintptr_t buggy_address = KASAN_SHADOW_TO_MEM( buggy_shadow_address );
  printf( "[KASan] ===================================================\r\n" );
  printf( "[KASan] ERROR: Invalid memory access: address %#"PRIxPTR", size %zu, write %d, ip %#"PRIxPTR"\r\n",
    addr, size, write ? 1 : 0, pc );
  kasan_print_shadow_memory( buggy_address, 3, 3 );
}
