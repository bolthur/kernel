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
 * @fn void* kasan_realloc_hook( void*, size_t, size_t )
 * @brief realloc hook
 * @param ptr
 * @param alignment
 * @param size
 * @return
 */
__no_sanitize void* kasan_realloc_hook( void* ptr, const size_t alignment, const size_t size ) {
  // for early heap skip asan
  if ( heap_get_state() == HEAP_INIT_EARLY ) {
    return heap_reallocate( ptr, alignment, size );
  }
  // handle no ptr
  if ( ! ptr ) {
    return kasan_aligned_alloc_hook( alignment, size );
  }
  // handle no size
  if ( ! size ) {
    kasan_free_hook( ptr );
    return nullptr;
  }
  // calculate size
  const size_t aligned_size = ( size + KASAN_SHADOW_MASK ) & ~KASAN_SHADOW_MASK;
  const size_t total_size = aligned_size + KASAN_HEAP_HEAD_REDZONE_SIZE + KASAN_HEAP_TAIL_REDZONE_SIZE;
  // handle in early
  if ( heap_address_is_in_early( ptr ) ) {
    // reallocate some block
    void* new_ptr = heap_reallocate( ptr, alignment, total_size );
    if ( ! new_ptr ) {
      return nullptr;
    }
    // populate kasan information
    auto const kasan_heap_header = ( kasan_heap_header_t* )ptr;
    kasan_heap_header->aligned_size = aligned_size;
    // unpoison and poison
    kasan_unpoison_shadow( ( uintptr_t )ptr + KASAN_HEAP_HEAD_REDZONE_SIZE, size );
    kasan_poison_shadow( ( uintptr_t )ptr, KASAN_HEAP_HEAD_REDZONE_SIZE, ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC, false );
    kasan_poison_shadow(( uintptr_t )ptr + KASAN_HEAP_HEAD_REDZONE_SIZE + aligned_size, KASAN_HEAP_TAIL_REDZONE_SIZE, ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC, false );
    return ( void* )( ( uintptr_t )ptr + KASAN_HEAP_HEAD_REDZONE_SIZE );
  }
  // get old size
  auto const old_header = ( kasan_heap_header_t* )( ( uintptr_t )ptr - KASAN_HEAP_HEAD_REDZONE_SIZE );
  // handle same size
  if ( old_header->aligned_size == aligned_size ) {
    return ptr;
  }
  // allocate some block
  void* new_ptr = heap_reallocate( ptr, alignment, total_size );
  if ( ! new_ptr ) {
    return nullptr;
  }
  // populate kasan information
  auto const new_header = ( kasan_heap_header_t* )ptr;
  // handle in place extended
  if ( new_header == old_header ) {
    kasan_unpoison_shadow( ( uintptr_t )new_ptr + KASAN_HEAP_HEAD_REDZONE_SIZE, old_header->aligned_size );
    kasan_unpoison_shadow( ( uintptr_t )new_ptr + KASAN_HEAP_HEAD_REDZONE_SIZE, aligned_size );
    kasan_poison_shadow( ( uintptr_t )new_ptr, KASAN_HEAP_HEAD_REDZONE_SIZE, ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC, false );
    kasan_poison_shadow( ( uintptr_t )new_ptr + KASAN_HEAP_HEAD_REDZONE_SIZE + aligned_size, KASAN_HEAP_TAIL_REDZONE_SIZE, ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC, false );
  // handle reallocated differently
  } else {
    kasan_poison_shadow( ( uintptr_t )ptr, old_header->aligned_size, ASAN_SHADOW_HEAP_FREE_MAGIC, false );
    kasan_unpoison_shadow( ( uintptr_t )new_ptr + KASAN_HEAP_HEAD_REDZONE_SIZE, aligned_size );
    kasan_poison_shadow( ( uintptr_t )new_ptr, KASAN_HEAP_HEAD_REDZONE_SIZE, ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC, false );
    kasan_poison_shadow( ( uintptr_t )new_ptr + KASAN_HEAP_HEAD_REDZONE_SIZE + aligned_size, KASAN_HEAP_TAIL_REDZONE_SIZE, ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC, false );
  }
  // populate aligned size
  new_header->aligned_size = aligned_size;
  // return newly allocated pointer
  return ( void* )( ( uintptr_t )new_ptr + KASAN_HEAP_HEAD_REDZONE_SIZE );
}
