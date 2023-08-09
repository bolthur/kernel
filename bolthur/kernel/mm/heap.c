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

#include <stdbool.h>
#include <stddef.h>
#include "../lib/inttypes.h"
#include "../lib/assert.h"
#include "../lib/string.h"
#if defined( PRINT_MM_HEAP )
  #include "../debug/debug.h"
#endif
#include "../mm/phys.h"
#include "../mm/virt.h"
#include "../mm/heap.h"
#include "../panic.h"
#include "../debug/debug.h"

void* dlmemalign( size_t, size_t );
void dlfree( void* );

/**
 * @brief Kernel heap
 */
heap_manager_t* kernel_heap = NULL;

/**
 * @fn bool heap_init_get(void)
 * @brief Getter for heap initialized flag
 *
 * @return
 */
bool heap_init_get( void ) {
  return ( bool )kernel_heap;
}

/**
 * @fn void heap_init(heap_init_state_t)
 * @brief new heap init implementation
 *
 * @param state
 */
void heap_init( heap_init_state_t state ) {
  if (
    // check for invalid state
    (
      HEAP_INIT_EARLY != state
      && HEAP_INIT_NORMAL != state
    // check for already gone further
    ) || (
      kernel_heap
      && kernel_heap->state > state
    )
  ) {
    return;
  }

  // handle normal heap init
  if ( HEAP_INIT_NORMAL == state ) {
    // assert kernel heap existence
    assert( kernel_heap );
    // allocate space for sbrk
    for (
      uintptr_t addr = HEAP_START;
      addr < HEAP_START + HEAP_MIN_SIZE;
      addr += PAGE_SIZE
    ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "Map %#"PRIxPTR" with random physical address\r\n", addr )
      #endif
      // map address
      assert( virt_map_address_random(
        virt_current_kernel_context,
        addr,
        VIRT_MEMORY_TYPE_NORMAL_NC,
        VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
      ) )
    }
    // set state
    kernel_heap->state = state;
    // skip rest
    return;
  }

  // start and end of early init
  uintptr_t start = ( uintptr_t )&__initial_heap_start;
  uintptr_t end = ( uintptr_t )&__initial_heap_end;
  // assert structure to be invalid
  assert( ! kernel_heap );

  // place right at the beginning
  kernel_heap = ( heap_manager_t* )&__initial_heap_start;
  // clear out space
  memset( kernel_heap, 0, sizeof( heap_manager_t ) );

  // place first block directly behind manager
  heap_block_t* block = ( heap_block_t* )(
    start + sizeof( heap_manager_t ) );
  // clear out space
  memset( block, 0, sizeof( heap_block_t ) );

  // set start beyond manager and block
  start += sizeof( heap_manager_t ) + sizeof( heap_block_t );
  // prepare management structure
  kernel_heap->start = start;
  kernel_heap->end = end;
  kernel_heap->free = block;
  kernel_heap->used = NULL;
  kernel_heap->state = state;

  // prepare block
  block->size = end - start;
  block->address = start;
  block->next = NULL;
  block->previous = NULL;
}

/**
 * @fn void heap_allocate*(size_t, size_t)
 * @brief
 *
 * @param alignment
 * @param size
 */
void* heap_allocate( size_t alignment, size_t size ) {
  // ensure that heap is initialized and size is valid
  if ( ! kernel_heap || 0 == size) {
    return NULL;
  }
  // handle normal state
  if ( HEAP_INIT_NORMAL == kernel_heap->state ) {
    return dlmemalign( alignment, size );
  }
  // try to find matching one
  heap_block_t* current = kernel_heap->free;
  while ( current ) {
    // handle to small
    if ( size > current->size ) {
      current = current->next;
      continue;
    }
    // save alignment result
    uintptr_t alignment_result = current->address % alignment;
    // handle case that no alignment adjustment is necessary
    if ( 0 == alignment_result ) {
      break;
    }
    // calculate initial alignment offset
    uintptr_t alignment_offset = alignment - alignment_result
      - sizeof( *current );
    // handle alignment with split possible
    if ( current->size > alignment_offset + size ) {
      break;
    }
    // get to next
    current = current->next;
  }
  // handle not enough free space
  if ( ! current ) {
    return NULL;
  }
  // change possible previous of next
  if ( current->next ) {
    current->next->previous = current->previous;
  }
  // change possible next of previous or free start
  if ( current->previous ) {
    current->previous->next = current->next;
  } else {
    kernel_heap->free = current->next;
  }
  // reset next and previous
  current->next = current->previous = NULL;

  // handle alignment
  uintptr_t alignment_result = current->address % alignment;
  if ( alignment_result ) {
    // calculate initial alignment offset
    uintptr_t alignment_offset = alignment - alignment_result
      - sizeof( *current );
    // new block with proper alignment
    heap_block_t* new_block = ( heap_block_t* )(
      ( uintptr_t )current->address + alignment_offset );
    // prepare new block
    new_block->address = ( uintptr_t )new_block + sizeof( *new_block );
    new_block->size = current->size - alignment_offset;
    new_block->next = NULL;
    new_block->previous = NULL;
    // update current
    current->size = ( uintptr_t )new_block - current->address;
    // prepend current to free list
    current->next = kernel_heap->free;
    if ( kernel_heap->free ) {
      kernel_heap->free->previous = current;
    }
    kernel_heap->free = current;
    // overwrite current after split
    current = new_block;
  }

  // check whether split is possible
  if ( current->size > size + sizeof( *current ) ) {
    heap_block_t* new_block = ( heap_block_t* )(
      current->address + size );
    // set size and address of new block
    new_block->size = current->size - size - sizeof( *new_block );
    new_block->address = ( uintptr_t )new_block + sizeof( *new_block );
    // prepend to free list
    new_block->next = kernel_heap->free;
    if ( kernel_heap->free ) {
      kernel_heap->free->previous = new_block;
    }
    kernel_heap->free = new_block;
    // reset size of current
    current->size = size;
  }

  // push to used
  current->next = kernel_heap->used;
  if ( kernel_heap->used ) {
    kernel_heap->used->previous = current;
  }
  kernel_heap->used = current;
  // return address
  return ( void* )current->address;
}

/**
 * @fn void heap_free(void*)
 * @brief heap free implementation
 *
 * @param addr
 */
void heap_free( void* addr ) {
  uintptr_t uaddr = ( uintptr_t )addr;
  // initial heap supports only simple free without block merging
  if (
    uaddr >= kernel_heap->start
    && uaddr <= kernel_heap->end
  ) {
    // try to find matching one
    heap_block_t* current = kernel_heap->used;
    while ( current ) {
      // handle "match"
      if ( current->address >= uaddr ) {
        break;
      }
      // get to next
      current = current->next;
    }
    // handle not found
    if ( ! current ) {
      return;
    }
    // change possible previous of next
    if ( current->next ) {
      current->next->previous = current->previous;
    }
    // change possible next of previous or free start
    if ( current->previous ) {
      current->previous->next = current->next;
    } else {
      kernel_heap->used = current->next;
    }
    // push to list
    kernel_heap->free->previous = current;
    current->next = kernel_heap->free;
    kernel_heap->free = current;
  }
  // use dlfree if normal state is setup
  if ( HEAP_INIT_NORMAL == kernel_heap->state ) {
    dlfree( addr );
  }
}

/**
 * @fn void heap_sbrk*(intptr_t)
 * @brief Implementation of sbrk used by dlmalloc
 *
 * @param increment
 *
 * @todo add support for decrease
 * @todo add check for some max heap which needs to be defined
 */
void* heap_sbrk( intptr_t increment ) {
  static uint8_t* heap_end = NULL;
  static uint8_t* max_heap = NULL;
  __unused static uint8_t* min_heap = NULL;
  // handle no virtual memory manager
  if (
    ! virt_init_get()
    || ! kernel_heap
    || HEAP_INIT_NORMAL != kernel_heap->state
  ) {
    // return error
    return ( void* )-1;
  }
  // initialize on first run
  if ( ! heap_end ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "sbrk init ( first call )!\r\n" )
    #endif
    heap_end = ( uint8_t* )HEAP_START;
    max_heap = ( uint8_t* )( HEAP_START + HEAP_MIN_SIZE );
    min_heap = ( uint8_t* )( HEAP_START + HEAP_MIN_SIZE );
  }
  // handle decrease
  if ( 0 > increment ) {
    PANIC( "sbrk doesn't support negative values!" )
    return ( void* )-1;
  }
  // save previous heap end
  uint8_t* prev_heap_end = heap_end;
  // increment
  heap_end += increment;
  // handle max reached
  if ( heap_end >= max_heap ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Try to extend heap area with size %#"PRIxPTR"\r\n", increment )
    #endif
    // transform max heap to uintptr_t
    uintptr_t max_heap_extend = ( uintptr_t )max_heap;
    // reset heap end
    heap_end = prev_heap_end;
    // extend heap space
    for (
      uintptr_t addr = max_heap_extend;
      addr < max_heap_extend + ( uintptr_t )increment;
      addr += PAGE_SIZE
    ) {
      // map address
      if ( ! virt_map_address_random(
        virt_current_kernel_context,
        addr,
        VIRT_MEMORY_TYPE_NORMAL_NC,
        VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
      ) ) {
        return ( void* )-1;
      }
      // clear area
      memset( ( void* )addr, 0, PAGE_SIZE );
      // update max heap address
      max_heap += PAGE_SIZE;
    }
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "max_heap = %p\r\n", max_heap )
    #endif
    // try again
    return heap_sbrk( increment );
  }
  // return previous heap end
  return prev_heap_end;
}
