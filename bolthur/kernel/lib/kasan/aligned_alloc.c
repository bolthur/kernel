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

#include <inttypes.h>

#include "kasan.h"
#include "../../panic.h"
#include "../../debug/debug.h"

/**
 * @fn void* kasan_aligned_alloc_hook(size_t, size_t)
 * @brief Aligned allocation hook
 * @param alignment
 * @param size
 * @return
 */
__no_sanitize void* kasan_aligned_alloc_hook( const size_t alignment, const size_t size ) {
  // for early heap skip asan
  if ( heap_get_state() == HEAP_INIT_EARLY ) {
    return heap_allocate( alignment, size );
  }
  kasan_heap_header_t* kasan_heap_header = NULL;
  const size_t aligned_size = ( size + KASAN_SHADOW_MASK ) & ~KASAN_SHADOW_MASK;
  const size_t total_size = aligned_size + KASAN_HEAP_HEAD_REDZONE_SIZE
    + KASAN_HEAP_TAIL_REDZONE_SIZE;
  //DEBUG_OUTPUT( "Allocating %#zx, original size %#zx\r\n", total_size, size )
  // allocate some block
  void* ptr = heap_allocate( alignment, total_size );
  if ( ! ptr ) {
    return NULL;
  }
  //DEBUG_OUTPUT( "ptr = %#"PRIxPTR"\r\n", ( uintptr_t )ptr )
  // populate kasan information
  kasan_heap_header = ( kasan_heap_header_t* )ptr;
  kasan_heap_header->aligned_size = aligned_size;

  kasan_unpoison_shadow( ( uintptr_t )ptr + KASAN_HEAP_HEAD_REDZONE_SIZE, size );
  kasan_poison_shadow( ( uintptr_t )ptr, KASAN_HEAP_HEAD_REDZONE_SIZE, ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC, false );
  kasan_poison_shadow(( uintptr_t )ptr + KASAN_HEAP_HEAD_REDZONE_SIZE + aligned_size, KASAN_HEAP_TAIL_REDZONE_SIZE, ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC, false );
  return ( void* )( ( uintptr_t )ptr + KASAN_HEAP_HEAD_REDZONE_SIZE );
}
