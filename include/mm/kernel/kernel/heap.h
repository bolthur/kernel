
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

#if ! defined( __MM_KERNEL_KERNEL_HEAP__ )
#define __MM_KERNEL_KERNEL_HEAP__

#include <stddef.h>
#include <stdbool.h>
#include <kernel/entry.h>

#if defined( ELF32 )
  #define HEAP_START 0xD0000000
  #define HEAP_MAX_SIZE 0xDFFFFFFF
  #define HEAP_MIN_SIZE 0x4000
#elif defined( ELF64 )
  #error "Heap not ready for x64"
#endif

typedef struct {
  vaddr_t start;
  vaddr_t end;
  size_t size;
} heap_t, *heap_ptr_t;

heap_ptr_t kernel_heap;

bool heap_initialized_get( void );
void heap_init( void );

#endif
