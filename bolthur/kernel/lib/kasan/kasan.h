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

#ifndef _LIB_KASAN_KASAN_H
#define _LIB_KASAN_KASAN_H

#include <stdint.h>
#include <stdbool.h>
#include "../../mm/heap.h"

#if defined( ELF32 )
  #define KASAN_SHADOW_MEMORY_OFFSET 0xC6000000
#elif defined( ELF64 )
  #error "Shadow memory offset not defined for 64 bit"
#endif

#define KASAN_CALLER_PC ( ( uintptr_t )__builtin_return_address( 0 ) )

#define KASAN_SHADOW_SHIFT 3
#define KASAN_SHADOW_GRANULE_SIZE ( 1UL << KASAN_SHADOW_SHIFT )
#define KASAN_SHADOW_MASK ( KASAN_SHADOW_GRANULE_SIZE - 1 )

#define ASAN_SHADOW_UNPOISONED_MAGIC 0x00
#define ASAN_SHADOW_RESERVED_MAGIC 0xff
#define ASAN_SHADOW_GLOBAL_REDZONE_MAGIC 0xf9
#define ASAN_SHADOW_HEAP_HEAD_REDZONE_MAGIC 0xfa
#define ASAN_SHADOW_HEAP_TAIL_REDZONE_MAGIC 0xfb
#define ASAN_SHADOW_HEAP_FREE_MAGIC 0xfd

extern uintptr_t kasan_shadow_memory_start;
extern uintptr_t kasan_shadow_memory_end;

typedef struct {
  size_t aligned_size;
  size_t dummy;
} kasan_heap_header_t;

#define KASAN_HEAP_HEAD_REDZONE_SIZE sizeof( kasan_heap_header_t )
#define KASAN_HEAP_TAIL_REDZONE_SIZE sizeof( kasan_heap_header_t )

#define KASAN_MEM_TO_SHADOW( addr ) ( ( ( addr ) >> KASAN_SHADOW_SHIFT ) + KASAN_SHADOW_MEMORY_OFFSET )
#define KASAN_SHADOW_TO_MEM( shadow ) ( ( ( shadow ) - KASAN_SHADOW_MEMORY_OFFSET ) << KASAN_SHADOW_SHIFT )

// printing related functions
void kasan_print_16_bytes_no_bug(const char*, uintptr_t );
void kasan_print_16_bytes_with_bug(const char*, uintptr_t, uintptr_t );
void kasan_print_shadow_memory( uintptr_t, uintptr_t, uintptr_t );
void kasan_bug_report( uintptr_t, size_t, uintptr_t, bool, uintptr_t );

// poison and check memory
uintptr_t kasan_get_poisoned_shadow_address( uintptr_t, size_t );
void kasan_poison_shadow( uintptr_t, size_t, uint8_t, bool );
void kasan_unpoison_shadow( uintptr_t, size_t );
int kasan_check_memory( uintptr_t, size_t, bool, uintptr_t );

// hook functions
void* kasan_aligned_alloc_hook( size_t, size_t );
void kasan_free_hook( void* );

// init
void kasan_init( void );

// asan functions
void __asan_store1_noabort( uintptr_t );
void __asan_store2_noabort( uintptr_t );
void __asan_store4_noabort( uintptr_t );
void __asan_store8_noabort( uintptr_t );
void __asan_store16_noabort( uintptr_t );
void __asan_storeN_noabort( uintptr_t, size_t  );

void __asan_load1_noabort( uintptr_t );
void __asan_load2_noabort( uintptr_t );
void __asan_load4_noabort( uintptr_t );
void __asan_load8_noabort( uintptr_t );
void __asan_load16_noabort( uintptr_t );
void __asan_loadN_noabort( uintptr_t, size_t  );

void __asan_handle_no_return( void );

#endif
