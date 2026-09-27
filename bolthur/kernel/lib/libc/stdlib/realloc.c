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

#include <stddef.h>
#include "../../stdlib.h"
#include "../../../mm/heap.h"
#include "../../kasan/kasan.h"

/**
 * @fn void* realloc(void*, size_t)
 * @brief Malloc implementation
 *
 * @param size size to allocate
 * @return void* allocated address or nullptr
 */
__allocator void* realloc( void* ptr, const size_t size ) {
  // sanitizer stuff
  #if defined( HAS_SANITIZER )
    return kasan_realloc_hook( ptr, alignof( max_align_t ), size );
  // no sanitizer stuff
  #else
    // no pointer just allocate
    if ( ! ptr ) {
      return malloc( size );
    }
    // no size just free
    if ( ! size ) {
      free( ptr );
      return nullptr;
    }
    // use heap allocation
    return heap_reallocate( ptr, alignof( max_align_t ), size );
  #endif
}
