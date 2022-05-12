/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#if ! defined( _MM_HEAP_H )
#define _MM_HEAP_H

#include <stddef.h>
#include <stdbool.h>
#include "../entry.h"

#if defined( ELF32 )
  #define HEAP_START 0xD0000000
  #define HEAP_MAX_SIZE 0xFFFFFFF
  #define HEAP_MIN_SIZE 0x10000
  #define HEAP_EXTENSION 0x1000
#elif defined( ELF64 )
  #error "Heap not ready for x64"
#endif

typedef enum {
  HEAP_INIT_EARLY = 0,
  HEAP_INIT_NORMAL,
  HEAP_INIT_SIZE,
} heap_init_state_t;

typedef struct heap_block heap_block_t;

typedef struct heap_block {
  size_t size;
  uintptr_t address;
  // pointer to next and previous block
  heap_block_t* next;
  heap_block_t* previous;
} heap_block_t;

typedef struct {
  uintptr_t start;
  uintptr_t end;
  heap_init_state_t state;
  // pointers for traversing list a like
  heap_block_t* used;
  heap_block_t* free;
} heap_manager_t;

extern uintptr_t __initial_heap_start;
extern uintptr_t __initial_heap_end;

bool heap_init_get( void );
void heap_init( heap_init_state_t );
void* heap_allocate( size_t, size_t );
void heap_free( void* );
void* heap_sbrk( intptr_t );

#endif
