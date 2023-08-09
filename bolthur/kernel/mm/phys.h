/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#ifndef _MM_PHYS_H
#define _MM_PHYS_H

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_PER_ENTRY ( sizeof( phys_bitmap_length ) * CHAR_BIT )
#define PAGE_INDEX( address ) ( address / PAGE_PER_ENTRY )
#define PAGE_OFFSET( address ) ( address % PAGE_PER_ENTRY )

#define PHYS_ALL_PAGES_OF_INDEX_USED 0xFFFFFFFF

#define PAGE_SIZE 0x1000
#define ROUND_DOWN_TO_FULL_PAGE( a ) \
  ( ( uintptr_t )( a ) & ( uintptr_t )( ~( ( PAGE_SIZE ) - 1 ) ) )
#define ROUND_UP_TO_FULL_PAGE( a ) \
  ( ( ( ( uintptr_t )( a ) & ( ( PAGE_SIZE ) - 1 ) ) ? ( PAGE_SIZE ) : 0 ) \
    + ROUND_DOWN_TO_FULL_PAGE( ( a ) ) )
#define ROUND_PAGE_OFFSET( a ) ( ( uintptr_t )( a ) & ( ( PAGE_SIZE ) -1 ) )

typedef enum {
  PHYS_MEMORY_TYPE_NORMAL,
  PHYS_MEMORY_TYPE_DMA,
} phys_memory_type_t;

extern uint32_t* phys_bitmap;
extern uint32_t* phys_bitmap_check;
extern uint32_t phys_bitmap_length;

extern uint32_t* phys_dma_bitmap;
extern uint32_t phys_dma_length;
extern uint64_t phys_dma_start;
extern uint64_t phys_dma_end;

void phys_init( void );
bool phys_dma_init( void );
bool phys_platform_init( void );

void phys_mark_page_used( uint64_t );
void phys_mark_page_free( uint64_t );
uint64_t phys_find_free_page_range( size_t, size_t, phys_memory_type_t );
void phys_free_page_range( uint64_t, size_t );
void phys_use_page_range( uint64_t, size_t );
uint64_t phys_find_free_page( size_t, phys_memory_type_t );
void phys_free_page( uint64_t );
bool phys_init_get( void );
bool phys_is_range_used( uint64_t, size_t );
void phys_mark_page_free_check( uint64_t );
void phys_free_page_range_check( uint64_t, size_t );
bool phys_free_check_only( uint64_t );
uint64_t phys_address_to_bus( uint64_t, size_t );

#endif
