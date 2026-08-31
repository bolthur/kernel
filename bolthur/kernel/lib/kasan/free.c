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

#include "kasan.h"

/**
 * @fn void kasan_free_hook(void*)
 * @brief kasan free hook
 * @param ptr
 */
__no_sanitize void kasan_free_hook( void* ptr ) {
  // handle invalid address
  if ( ! ptr ) {
    return;
  }
  // for early heap skip asan
  if ( heap_get_state() == HEAP_INIT_EARLY ) {
    heap_free( ptr );
    return;
  }
  // translate to kasan heap header
  kasan_heap_header_t* kasan_heap_header = ( kasan_heap_header_t* )(
    ( uintptr_t )ptr - KASAN_HEAP_HEAD_REDZONE_SIZE );
  // extract aligned size
  const size_t aligned_size = kasan_heap_header->aligned_size;
  // free address
  heap_free( kasan_heap_header );
  // poison shadow
  kasan_poison_shadow( ( uintptr_t )ptr, aligned_size, ASAN_SHADOW_HEAP_FREE_MAGIC, false );
}
