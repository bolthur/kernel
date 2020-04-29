
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <string.h>
#include <assert.h>
#include <avl.h>
#include <core/debug/debug.h>
#include <core/panic.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <core/mm/heap.h>

/**
 * @brief Kernel heap
 */
heap_manager_ptr_t kernel_heap = NULL;

/**
 * @brief Get the free address tree object
 *
 * @param state
 * @param heap
 * @return avl_tree_ptr_t
 */
static avl_tree_ptr_t get_free_address_tree(
  heap_init_state_t state,
  heap_manager_ptr_t heap
) {
  if (
    state == HEAP_INIT_NONE
    || NULL == heap
  ) {
    return NULL;
  }

  return &heap->free_address[ state ];
}

/**
 * @brief Get the free size tree object
 *
 * @param state
 * @param heap
 * @return avl_tree_ptr_t
 */
static avl_tree_ptr_t get_free_size_tree(
  heap_init_state_t state,
  heap_manager_ptr_t heap
) {
  if (
    state == HEAP_INIT_NONE
    || NULL == heap
  ) {
    return NULL;
  }

  return &heap->free_size[ state ];
}

/**
 * @brief Get the used area tree object
 *
 * @param state
 * @param heap
 * @return avl_tree_ptr_t
 */
static avl_tree_ptr_t get_used_area_tree(
  heap_init_state_t state,
  heap_manager_ptr_t heap
) {
  if (
    state == HEAP_INIT_NONE
    || NULL == heap
  ) {
    return NULL;
  }

  return &heap->used_area[ state ];
}

/**
 * @brief Compare address callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_address_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = 0x%08p, b = 0x%08p\r\n", a, b );
    DEBUG_OUTPUT( "a->data = 0x%08p, b->data = 0x%08p\r\n", a->data, b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( a->data > b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( b->data > a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Compare address callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_size_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = 0x%08p, b = 0x%08p\r\n", a, b );
    DEBUG_OUTPUT( "a->data = 0x%08p, b->data = 0x%08p\r\n", a->data, b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( ( size_t )a->data > ( size_t )b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( size_t )b->data > ( size_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Helper to prepare a block
 *
 * @param block block to prepare
 * @param addr address for block
 * @param size size for block
 */
static void prepare_block(
  heap_block_ptr_t block,
  uintptr_t addr,
  size_t size
) {
  // clear memory
  memset( ( void* )block, 0, size + sizeof( heap_block_t ) );

  // prepare block itself
  block->size = size;
  block->address = addr;

  // prepare tree nodes
  avl_prepare_node( &block->node_address, ( void* )addr );
  avl_prepare_node( &block->node_size, ( void* )size );
}

/**
 * @brief Internal method for extending the heap
 */
static void extend_heap_space( void ) {
  avl_node_ptr_t max, max_used, max_free;
  heap_block_ptr_t block, free_block;
  uintptr_t heap_end;
  size_t heap_size;
  avl_tree_ptr_t used_area, free_address, free_size;

  // stop if not setup
  if (
    NULL == kernel_heap
    || HEAP_INIT_NORMAL != kernel_heap->state
  ) {
    return;
  }

  // get correct trees
  used_area = get_used_area_tree( kernel_heap->state, kernel_heap );
  free_address = get_free_address_tree( kernel_heap->state, kernel_heap );
  free_size = get_free_size_tree( kernel_heap->state, kernel_heap );

  // get max from both trees
  max = NULL;
  max_used = avl_get_max( used_area->root );
  max_free = avl_get_max( free_address->root );

  // determine max node to use
  if ( NULL != max_used && NULL != max_free ) {
    if ( max_free->data > max_used->data ) {
      max = max_free;
    } else {
      max = max_used;
    }
  } else if ( NULL != max_used && NULL == max_free ) {
    max = max_used;
  } else if ( NULL != max_free && NULL == max_used ) {
    max = max_free;
  }

  // assert set max
  assert( NULL != max );

  // get block
  block = HEAP_GET_BLOCK_ADDRESS( max );
  // get heap end and current size
  heap_end = block->address + block->size;
  heap_size = heap_end - HEAP_START;
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "End: 0x%08x, Size: 0x%08x, New size: 0x%08x\r\n",
      heap_end,
      heap_size,
      heap_size + HEAP_EXTENSION
    );
  #endif
  // assert size against max size
  assert( heap_size + HEAP_EXTENSION < HEAP_MAX_SIZE );

  // map heap address space
  for (
    uintptr_t addr = heap_end;
    addr < heap_end + HEAP_EXTENSION;
    addr += PAGE_SIZE
  ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map 0x%08x with random physical address\r\n", addr );
    #endif

    // map address
    virt_map_address_random(
      kernel_context,
      addr,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_NON_EXECUTABLE );

    // clear area
    memset( ( void* )addr, 0, PAGE_SIZE );
  }

  // extend free block
  if ( max == max_free ) {
    // set free block to current block
    free_block = block;

    // remove node from free tree and size tree
    avl_remove_by_data( free_address, free_block->node_address.data );
    avl_remove_by_node( free_size, &free_block->node_size );

    // extend size
    free_block->size += HEAP_EXTENSION;
  // Create total new block
  } else {
    // create free block
    free_block = ( heap_block_ptr_t )heap_end;

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Created free block at 0x%08x\r\n", free_block );
    #endif

    // prepare free block
    free_block->address = ( uintptr_t )free_block + sizeof( heap_block_t );
    free_block->size = HEAP_EXTENSION;
  }

  // populate node data
  avl_prepare_node( &free_block->node_address, ( void* )free_block->address );
  avl_prepare_node( &free_block->node_size, ( void* )free_block->size );

  // insert into free trees
  avl_insert_by_node( free_address, &free_block->node_address );
  avl_insert_by_node( free_size, &free_block->node_size );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif
}

/**
 * @brief Helper to shrink heap if possible
 */
static void shrink_heap_space( void ) {
  heap_block_ptr_t max_block;
  size_t max_end, expand_size, to_remove;
  avl_node_ptr_t max_node;
  __maybe_unused avl_tree_ptr_t used_area;
  avl_tree_ptr_t free_address, free_size;

  // stop if not setup
  if (
    NULL == kernel_heap
    || HEAP_INIT_NORMAL != kernel_heap->state
  ) {
    return;
  }

  // get correct trees
  used_area = get_used_area_tree( kernel_heap->state, kernel_heap );
  free_address = get_free_address_tree( kernel_heap->state, kernel_heap );
  free_size = get_free_size_tree( kernel_heap->state, kernel_heap );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Try to shrink heap!\r\n" );
  #endif

  // get node on right side
  max_node = avl_get_max( free_address->root );
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_node = 0x%08x\r\n", max_node );
  #endif
  // skip if there is nothing
  if ( NULL == max_node ) {
    return;
  }

  // get block
  max_block = HEAP_GET_BLOCK_ADDRESS( max_node );
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_block = 0x%08x\r\n", max_block );
  #endif

  // determine end of block
  max_end = ( size_t )max_block->address + max_block->size;
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_end = 0x%08x\r\n", max_end );
  #endif
  // do nothing if not yet expanded
  if ( HEAP_START + HEAP_MIN_SIZE >= max_end ) {
    return;
  }

  // determine expand size
  expand_size = max_end - ( HEAP_START + HEAP_MIN_SIZE );
  // do nothing if expand size is below the expand unit
  if (
    HEAP_EXTENSION > expand_size
    || max_block->size < HEAP_EXTENSION
  ) {
    return;
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "expand_size = 0x%08x\r\n", expand_size );
  #endif

  // remove from free address
  avl_remove_by_data( free_address, max_block->node_address.data );
  // remove from free size tree
  avl_remove_by_node( free_size, &max_block->node_size );

  // adjust block size
  to_remove = 0;
  while ( max_block->size > HEAP_EXTENSION ) {
    to_remove += HEAP_EXTENSION;
    max_block->size -= HEAP_EXTENSION;
  }
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "to_remove = 0x%08x\r\n", to_remove );
  #endif
  // prepare blocks after adjustment
  prepare_block( max_block, max_block->address, max_block->size );
  // reinsert block
  avl_insert_by_node( free_address, &max_block->node_address );
  avl_insert_by_node( free_size, &max_block->node_size );

  // free up virtual memory
  for (
    uintptr_t start = max_block->address + max_block->size;
    start < max_end;
    start += PAGE_SIZE
  ) {
    virt_unmap_address( kernel_context, start, true );
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif
}

/**
 * @brief Helper to check whether two blocks are mergable
 *
 * @param a block a
 * @param b block b
 * @return bool
 */
static bool mergable( heap_block_ptr_t a, heap_block_ptr_t b ) {
  uintptr_t a_end, b_end;

  // calculate end of a and b
  a_end = a->address + a->size;
  b_end = b->address + b->size;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "a = 0x%08x, a->address = 0x%08x, size = %d, a_end = 0x%08x\r\n",
      a, a->address, a->size, a_end
    );
    DEBUG_OUTPUT(
      "b = 0x%08x, b->address = 0x%08x, size = %d, b_end = 0x%08x\r\n",
      b, b->address, b->size, b_end
    );
  #endif

  // mergable if following directly
  return a_end == ( uintptr_t )b || b_end == ( uintptr_t )a;
}

/**
 * @brief Method merges two blocks if possible
 *
 * @param a block a
 * @param b block b
 * @return heap_block_ptr_t
 */
static heap_block_ptr_t merge( heap_block_ptr_t a, heap_block_ptr_t b ) {
  uintptr_t a_end, b_end;
  heap_block_ptr_t to_insert = NULL;
  avl_tree_ptr_t free_address, free_size;

  // skip if not mergable
  if ( true != mergable( a, b ) ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "a ( 0x%08x ) not mergable with b ( 0x%08x )", a, b );
    #endif
    // return NULL
    return NULL;
  }

  // calculate end of a and b
  a_end = a->address + a->size;
  b_end = b->address + b->size;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "a = 0x%08x, a->address = 0x%08x, size = %d, a_end = 0x%08x\r\n",
      a, a->address, a->size, a_end
    );
    DEBUG_OUTPUT(
      "b = 0x%08x, b->address = 0x%08x, size = %d, b_end = 0x%08x\r\n",
      b, b->address, b->size, b_end
    );
  #endif

  // get correct trees
  free_address = get_free_address_tree( kernel_heap->state, kernel_heap );
  free_size = get_free_size_tree( kernel_heap->state, kernel_heap );

  // remove from free address
  avl_remove_by_data( free_address, a->node_address.data );
  avl_remove_by_data( free_address, b->node_address.data );
  // remove from free size tree
  avl_remove_by_node( free_size, &a->node_size );
  avl_remove_by_node( free_size, &b->node_size );

  // merge a into b
  if ( b_end == ( uintptr_t )a ) {
    b->size += sizeof( heap_block_t ) + a->size;
    to_insert = b;
  // merge b into a
  } else if ( a_end == ( uintptr_t )b ) {
    a->size += sizeof( heap_block_t ) + b->size;
    to_insert = a;
  }

  // assert pointer to inser
  assert( NULL != to_insert );
  // prepare blocks of element to insert
  prepare_block( to_insert, to_insert->address, to_insert->size );
  // insert merged node
  avl_insert_by_node( free_address, &to_insert->node_address );
  avl_insert_by_node( free_size, &to_insert->node_size );
  // return pointer to merged node
  return to_insert;
}

/**
 * @brief Initialize heap
 *
 * @todo check normal init after early init
 * @todo find issue ocurring somewhere here on real hardware
 */
void heap_init( heap_init_state_t state ) {
  // correct state
  assert( HEAP_INIT_EARLY == state || HEAP_INIT_NORMAL == state );
  // check state if already initialized
  if ( NULL != kernel_heap ) {
    assert( kernel_heap->state < state );
  }

  // start and end of early init
  uintptr_t initial_start = ( uintptr_t )&__initial_heap_start;
  uintptr_t initial_end = ( uintptr_t )&__initial_heap_end;

  // offset for first free block
  uint32_t offset = 0;
  // start and min size
  uintptr_t start = HEAP_START;
  uintptr_t min_size = HEAP_MIN_SIZE;
  // handle early init
  if ( HEAP_INIT_EARLY == state ) {
    start = initial_start;
    min_size = initial_end - initial_start;
  }
  // heap to be created
  heap_manager_ptr_t heap = NULL;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "Heap start: 0x%08x, Heap end: 0x%08x\r\n",
      start,
      start + min_size
    );
    DEBUG_OUTPUT( "state = %d\r\n", state );
  #endif

  // map heap address space
  if ( HEAP_INIT_NORMAL == state ) {
    for (
      uintptr_t addr = start;
      addr < start + min_size;
      addr += PAGE_SIZE
    ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "Map 0x%08x with random physical address\r\n", addr );
      #endif

      // map address
      virt_map_address_random(
        kernel_context,
        addr,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_NON_EXECUTABLE );
    }
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Clearing out heap area\r\n" );
  #endif
  // erase kernel heap section
  memset( ( void* )start, 0, min_size );

  // set kernel heap and increase offset
  if ( NULL == kernel_heap ) {
    heap = ( heap_manager_ptr_t )start;
    offset += sizeof( heap_manager_t );
  } else {
    heap = kernel_heap;
  }


  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Placed heap management at 0x%08x\r\n", heap );
    DEBUG_OUTPUT( "offset: %d\r\n", offset );
  #endif

  // initialize management structure
  if ( NULL == kernel_heap ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Preparing heap management structure\r\n" );
    #endif
    // clear memory
    memset( ( void* )heap, 0, sizeof( heap_manager_t ) );
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Setting tree callbacks\r\n" );
  #endif
  // populate trees
  heap->free_address[ state ].compare = compare_address_callback;
  heap->used_area[ state ].compare = compare_address_callback;
  heap->free_size[ state ].compare = compare_size_callback;

  // create free block
  heap_block_ptr_t free_block = ( heap_block_ptr_t )( start + offset );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Placing free block at 0x%08x\r\n", free_block );
  #endif

  // increase offset
  offset += sizeof( heap_block_t );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Heap offset: %d\r\n", offset );
    DEBUG_OUTPUT( "Preparing and inserting free block\r\n" );
  #endif

  // prepare free block
  prepare_block( free_block, start + offset, min_size - offset );
  // insert into free trees
  avl_insert_by_node( &heap->free_address[ state ], &free_block->node_address );
  avl_insert_by_node( &heap->free_size[ state ], &free_block->node_size );

  // finally set state
  heap->state = state;
  // set kernel heap global if null
  if ( NULL == kernel_heap ) {
    kernel_heap = heap;
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area[ state ] );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address[ state ] );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size[ state ] );
  #endif
}

/**
 * @brief Getter for heap initialized flag
 *
 * @return true
 * @return false
 */
bool heap_init_get( void ) {
  return NULL != kernel_heap;
}

/**
 * @brief Allocate block within heap
 *
 * @param alignment memory alignment
 * @param size size to allocate
 * @return uintptr_t address of allocated block
 */
uintptr_t heap_allocate_block( size_t alignment, size_t size ) {
  // variables
  heap_block_ptr_t current, new, following, previous;
  avl_node_ptr_t address_node;
  size_t real_size, alignment_offset;
  bool split;
  avl_tree_ptr_t used_area, free_address, free_size;

  // stop if not setup
  if ( NULL == kernel_heap ) {
    return 0;
  }

  // calculate real size
  real_size = size + sizeof( heap_block_t );

  // get correct trees
  used_area = get_used_area_tree( kernel_heap->state, kernel_heap );
  free_address = get_free_address_tree( kernel_heap->state, kernel_heap );
  free_size = get_free_size_tree( kernel_heap->state, kernel_heap );

  // Try to find one that matches by size
  address_node = avl_find_by_data( free_size, ( void* )size );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "address_node = 0x%08x\r\n", address_node );
  #endif
  // check alignment for possible matching block
  if ( NULL != address_node ) {
    // get block pointer
    current = HEAP_GET_BLOCK_SIZE( address_node );

    // check for not matching alignment
    if ( current->address % alignment ) {
      // set address node to null on alignment mismatch
      address_node = NULL;
    }
  }

  // Check for no matching node has been found
  if ( NULL == address_node ) {
    // get max node
    address_node = avl_get_max( free_size->root );
    // check for error or empty
    if ( NULL == address_node ) {
      // no expansion within early heap state
      if ( HEAP_INIT_EARLY == kernel_heap->state ) {
        // debug output
        #if defined( PRINT_MM_HEAP )
          DEBUG_OUTPUT( "No max node found, tree empty\r\n" );
        #endif
        // return invalid
        return 0;
      }
      // extend heap
      extend_heap_space();
      // try another allocation
      return heap_allocate_block( alignment, size );
    }
  }

  // get block to split
  current = HEAP_GET_BLOCK_SIZE( address_node );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "address_node = 0x%08x\r\n", address_node );
    DEBUG_OUTPUT( "current->size = %d\r\n", current->size );
    DEBUG_OUTPUT( "current->address = 0x%08x\r\n", current->address );
  #endif

  // split flag
  split = current->size != size;
  // check for enough size for split
  if ( split && current->size <= real_size ) {
    // no expansion within early heap state
    if ( HEAP_INIT_EARLY == kernel_heap->state ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "Not enough space, existing size: %d, real size = %d\r\n",
          current->size,
          real_size
        );
      #endif
      // return 0
      return 0;
    }
    // extend heap
    extend_heap_space();
    // try another allocation
    return heap_allocate_block( alignment, size );
  }

  // possible alignment offset
  alignment_offset = 0;
  // handle alignment
  if ( current->address % alignment ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "alignment_offset = %d\r\n", alignment_offset );
    #endif

    // determine alignment offset
    alignment_offset = ( alignment - ( current->address % alignment ) );
    do {
      alignment_offset += alignment;
    } while ( alignment_offset < real_size );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "alignment_offset = %d\r\n", alignment_offset );
    #endif

    // subtract heap block size
    alignment_offset -= sizeof( heap_block_t );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "alignment_offset = %x\r\n", alignment_offset );
    #endif

    // check for extension is necessary
    if (
      // offset bigger than size
      current->size < alignment_offset
      // not enough space with offset
      || current->size - alignment_offset <= real_size
      // check for enough following space
      || (
        current->size - alignment_offset > real_size
        && current->size - alignment_offset < real_size + sizeof( heap_block_t )
      )
    ) {
      // no expansion within early heap state
      if ( HEAP_INIT_EARLY == kernel_heap->state ) {
        // debug output
        #if defined( PRINT_MM_HEAP )
          DEBUG_OUTPUT( "Not enough space with alignment\r\n" );
        #endif
        // return invalid
        return 0;
      }
      // extend heap
      extend_heap_space();
      // try another allocation
      return heap_allocate_block( alignment, size );
    }

    // set split flag
    split = true;
  }

  // remove nodes from free trees
  avl_remove_by_data( free_address, current->node_address.data );
  avl_remove_by_node( free_size, &current->node_size );

  if ( split && 0 == alignment_offset ) {
    // calculate remaining size
    uint32_t remaining_size = current->size - real_size;
    uintptr_t new_start = ( uintptr_t )current + real_size;

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "block->size = %d, remaining_size = %d, new_start 0x%08x\r\n",
        current->size, remaining_size, new_start );
    #endif

    // prevent unaligned access
    if ( new_start % 4 ) {
      // increment real size by necessary alignment offset
      real_size += ( new_start % 4 );
      // recalculate remaining size
      remaining_size = current->size - real_size;
      // recalculate new start
      new_start = ( uintptr_t )current + real_size;
    }

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "block->size = %d, remaining_size = %d, new_start 0x%08x\r\n",
        current->size, remaining_size, new_start );
    #endif

    // place new block before
    new = current;

    // move block
    current = ( heap_block_ptr_t )new_start;
    // prepare block
    prepare_block(
      current, ( uintptr_t )current + sizeof( heap_block_t ), remaining_size );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "block->size = %d\r\n", current->size );
      DEBUG_OUTPUT( "block->address = 0x%08x\r\n", current->address );
    #endif

    // insert nodes at free trees
    avl_insert_by_node( free_address, &current->node_address );
    avl_insert_by_node( free_size, &current->node_size );
  // split with alignment offset
  } else if ( split && 0 < alignment_offset ) {
    // determine previous block
    previous = current;
    // determine new block for usage
    new = ( heap_block_ptr_t )(
      ( uintptr_t )current->address + alignment_offset );

    // calculate size for check for following has to be created
    size_t check_following_size =
      alignment_offset + real_size + sizeof( heap_block_t );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "check_following_size = %d\r\n", check_following_size );
    #endif

    // create and insert following block
    if ( current->size > check_following_size ) {
      // determine following block
      following = ( heap_block_ptr_t )( ( uintptr_t )new + real_size );

      // prepare block
      prepare_block(
        following,
        ( uintptr_t )following + sizeof( heap_block_t ),
        current->size - check_following_size );

      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "following->size = %d\r\n", following->size );
        DEBUG_OUTPUT( "following->address = 0x%08x\r\n", following->address );
      #endif

      // insert nodes at free trees
      avl_insert_by_node( free_address, &following->node_address );
      avl_insert_by_node( free_size, &following->node_size );
    }

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "previous->size = %d\r\n", previous->size );
      DEBUG_OUTPUT( "previous->address = 0x%08x\r\n", previous->address );
    #endif

    // prepare previous block
    prepare_block(
      previous, previous->address, ( uintptr_t )new - previous->address );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "previous->size = %d\r\n", previous->size );
      DEBUG_OUTPUT( "previous->address = 0x%08x\r\n", previous->address );
    #endif

    // insert nodes at free trees
    avl_insert_by_node( free_address, &previous->node_address );
    avl_insert_by_node( free_size, &previous->node_size );
  // found matching node
  } else {
    new = current;
  }

  // prepare block
  prepare_block( new, ( uintptr_t )new + sizeof( heap_block_t ), size );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "new->size = %d\r\n", new->size );
    DEBUG_OUTPUT( "new->address = 0x%08x\r\n", new->address );
  #endif

  // insert at used block
  avl_insert_by_node( used_area, &new->node_address );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  // return address of block
  return new->address;
}

/**
 * @brief Free block within heap
 *
 * @param addr address to free
 */
void heap_free_block( uintptr_t addr ) {
  // variables
  avl_node_ptr_t address_node, parent_node, sibling_node;
  heap_block_ptr_t current_block, parent_block, sibling_block;
  avl_tree_ptr_t used_area, free_address, free_size;

  // stop if not setup
  if ( NULL == kernel_heap ) {
    return;
  }

  // start and end of early init
  uintptr_t initial_start = ( uintptr_t )&__initial_heap_start;
  uintptr_t initial_end = ( uintptr_t )&__initial_heap_end;

  // consider block from early setup
  if ( addr >= initial_start && addr <= initial_end ) {
    used_area = get_used_area_tree( HEAP_INIT_EARLY, kernel_heap );
    free_address = get_free_address_tree( HEAP_INIT_EARLY, kernel_heap );
    free_size = get_free_size_tree( HEAP_INIT_EARLY, kernel_heap );
  // else use heap state tree
  } else {
    used_area = get_used_area_tree( kernel_heap->state, kernel_heap );
    free_address = get_free_address_tree( kernel_heap->state, kernel_heap );
    free_size = get_free_size_tree( kernel_heap->state, kernel_heap );
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "addr = 0x%08p\r\n", addr );
  #endif

  // finde node by address within tree
  address_node = avl_find_by_data( used_area, ( void* )addr );

  // skip if nothing has been found
  if ( NULL == address_node ) {
    return;
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "address_node = 0x%08p, data = 0x%08p\r\n",
      address_node,
      address_node->data
    );
  #endif

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  // get memory block
  current_block = HEAP_GET_BLOCK_ADDRESS( address_node );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "current_block = 0x%08x\r\n", current_block );
  #endif

  // remove node from used block
  avl_remove_by_data( used_area, address_node->data );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "removed 0x%08x from tree\r\n", &current_block->node_address );

    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  // prepare block
  prepare_block( current_block, current_block->address, current_block->size );

  // insert nodes
  avl_insert_by_node( free_address, &current_block->node_address );
  avl_insert_by_node( free_size, &current_block->node_size );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  do {
    // left sibling
    sibling_node = current_block->node_address.left;
    // try to merge if not null
    if ( NULL != sibling_node ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "possible sibling for merge = 0x%08x\r\n", sibling_node );
        DEBUG_OUTPUT( "sibling->data = 0x%08x\r\n", sibling_node->data );
        DEBUG_OUTPUT( "current_block = 0x%08x\r\n", ( uintptr_t )current_block );
      #endif
      // get block structure of sibling
      sibling_block = HEAP_GET_BLOCK_ADDRESS( sibling_node );
      // merge if possible
      current_block = merge( current_block, sibling_block );
      // handle null return
      if ( NULL == current_block ) {
        break;
      }
    }

    // right sibling
    sibling_node = current_block->node_address.right;
    // try to merge if not null
    if ( NULL != sibling_node ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "possible sibling for merge = 0x%08x\r\n", sibling_node );
        DEBUG_OUTPUT( "sibling->data = 0x%08x\r\n", sibling_node->data );
        DEBUG_OUTPUT( "current_block = 0x%08x\r\n", ( uintptr_t )current_block );
      #endif
      // get block structure of sibling
      sibling_block = HEAP_GET_BLOCK_ADDRESS( sibling_node );
      // merge if possible
      current_block = merge( current_block, sibling_block );
      // handle null return
      if ( NULL == current_block ) {
        break;
      }
    }

    // find parent
    parent_node = avl_find_parent_by_data(
      free_address,
      current_block->node_address.data
    );
    // try to merge if not null
    if ( NULL != parent_node ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "possible parent for merge = 0x%08x\r\n", parent_node );
        DEBUG_OUTPUT( "parent->data = 0x%08x\r\n", parent_node->data );
      #endif
      // get block structure of parent_node
      parent_block = HEAP_GET_BLOCK_ADDRESS( parent_node );
      // merge if possible
      current_block = merge( current_block, parent_block );
    }
  } while ( NULL != parent_node && NULL != current_block );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  // Try to shrink heap if possible
  if ( HEAP_INIT_NORMAL == kernel_heap->state ) {
    shrink_heap_space();
  }
}
