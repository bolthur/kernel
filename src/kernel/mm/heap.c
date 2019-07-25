
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

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <avl.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>

/**
 * @brief Kernel heap
 */
heap_manager_ptr_t kernel_heap = NULL;

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

  // get max from both trees
  max = NULL;
  max_used = avl_get_max( kernel_heap->used_area.root );
  max_free = avl_get_max( kernel_heap->free_address.root );

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
  block = GET_BLOCK_ADDRESS( max );
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
      kernel_context, addr, PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );

    // clear area
    memset( ( void* )addr, 0, PAGE_SIZE );
  }

  // extend free block
  if ( max == max_free ) {
    // set free block to current block
    free_block = block;

    // remove node from free tree and size tree
    avl_remove_by_data( &kernel_heap->free_address, free_block->node_address.data );
    avl_remove_by_node( &kernel_heap->free_size, &free_block->node_size );

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
  avl_insert_by_node( &kernel_heap->free_address, &free_block->node_address );
  avl_insert_by_node( &kernel_heap->free_size, &free_block->node_size );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
  #endif
}

/**
 * @brief Helper to shrink heap if possible
 */
static void shrink_heap_space( void ) {
  heap_block_ptr_t max_block;
  size_t max_end, expand_size, to_remove;
  avl_node_ptr_t max_node;

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s\r\n", "Try to shrink heap!" );
  #endif

  // get node on right side
  max_node = avl_get_max( kernel_heap->free_address.root );
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_node = 0x%08x\r\n", max_node );
  #endif
  // skip if there is nothing
  if ( NULL == max_node ) {
    return;
  }

  // get block
  max_block = GET_BLOCK_ADDRESS( max_node );
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
  avl_remove_by_data( &kernel_heap->free_address, max_block->node_address.data );
  // remove from free size tree
  avl_remove_by_node( &kernel_heap->free_size, &max_block->node_size );

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
  avl_insert_by_node( &kernel_heap->free_address, &max_block->node_address );
  avl_insert_by_node( &kernel_heap->free_size, &max_block->node_size );

  // free up virtual memory
  for (
    uintptr_t start = max_block->address + max_block->size;
    start < max_end;
    start += PAGE_SIZE
  ) {
    virt_unmap_address( kernel_context, start );
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
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
  heap_block_ptr_t to_insert;

  // skip if not mergable
  if ( true != mergable( a, b ) ) {
    return NULL;
  }
  // calculate end of a and b
  a_end = a->address + a->size;
  b_end = b->address + b->size;

  // remove from free address
  avl_remove_by_data( &kernel_heap->free_address, a->node_address.data );
  avl_remove_by_data( &kernel_heap->free_address, b->node_address.data );
  // remove from free size tree
  avl_remove_by_node( &kernel_heap->free_size, &a->node_size );
  avl_remove_by_node( &kernel_heap->free_size, &b->node_size );

  // merge a into b
  if ( b_end == ( uintptr_t )a ) {
    b->size += sizeof( heap_block_t ) + a->size;
    to_insert = b;
  // merge b into a
  } else if ( a_end == ( uintptr_t )b ) {
    a->size += sizeof( heap_block_t ) + b->size;
    to_insert = a;
  // unsupported case should not occur
  } else {
    PANIC( "Blocks not mergable!" );
  }

  // prepare blocks of element to insert
  prepare_block( to_insert, to_insert->address, to_insert->size );
  // insert merged node
  avl_insert_by_node( &kernel_heap->free_address, &to_insert->node_address );
  avl_insert_by_node( &kernel_heap->free_size, &to_insert->node_size );
  // return pointer to merged node
  return to_insert;
}

/**
 * @brief Initialize heap
 */
void heap_init( void ) {
  // assert not initialized
  assert( NULL == kernel_heap );

  // offset for first free block
  uint32_t offset = 0;
  uintptr_t start = HEAP_START;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "Heap start: 0x%08x, Heap end: 0x%08x\r\n",
      HEAP_START,
      HEAP_START + HEAP_MIN_SIZE
    );
  #endif

  // map heap address space
  for (
    uintptr_t addr = start;
    addr < HEAP_START + HEAP_MIN_SIZE;
    addr += PAGE_SIZE
  ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map 0x%08x with random physical address\r\n", addr );
    #endif

    // map address
    virt_map_address_random(
      kernel_context, addr, PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "heap start: 0x%08x\r\n", HEAP_START );
  #endif

  // erase kernel heap section
  memset( ( void* )HEAP_START, 0, HEAP_MIN_SIZE );

  // set kernel heap and increase offset
  kernel_heap = ( heap_manager_ptr_t )HEAP_START;
  offset += sizeof( heap_manager_t );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "HEAP_START content: 0x%08x\t\n",
      *( ( uint32_t* )HEAP_START )
    );
    DEBUG_OUTPUT( "Placed heap management at 0x%08x\r\n", kernel_heap );
    DEBUG_OUTPUT( "offset: %d\r\n", offset );
  #endif

  // initialize management structure
  memset( ( void* )kernel_heap, 0, sizeof( heap_manager_t ) );

  // populate trees
  kernel_heap->free_address.compare = compare_address_callback;
  kernel_heap->used_area.compare = compare_address_callback;
  kernel_heap->free_size.compare = compare_size_callback;

  // create free block
  heap_block_ptr_t free_block = ( heap_block_ptr_t )( HEAP_START + offset );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Created free block at 0x%08x\r\n", free_block );
  #endif

  // increase offset
  offset += sizeof( heap_block_t );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Heap offset: %d\r\n", offset );
  #endif

  // prepare free block
  prepare_block( free_block, HEAP_START + offset, HEAP_MIN_SIZE - offset );

  // insert into free tree
  avl_insert_by_node( &kernel_heap->free_address, &free_block->node_address );
  avl_insert_by_node( &kernel_heap->free_size, &free_block->node_size );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
  #endif
}

/**
 * @brief Getter for heap initialized flag
 *
 * @return true
 * @return false
 */
bool heap_initialized_get( void ) {
  return NULL != kernel_heap;
}

/**
 * @brief Allocate block within heap
 *
 * @param size size to allocate
 * @return uintptr_t address of allocated block
 */
uintptr_t heap_allocate_block( size_t size ) {
  // variables
  heap_block_ptr_t block, new_block;
  avl_node_ptr_t address_node;
  size_t real_size;

  // calculate real size
  real_size = size + sizeof( heap_block_t );

  // Try to find one that matches by size
  address_node = avl_find_by_data(
    &kernel_heap->free_size,
    ( void* )size
  );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "address_node = 0x%08x\r\n", address_node );
  #endif

  // Check for no node has been found
  if ( NULL == address_node ) {
    // get max node
    address_node = avl_get_max( kernel_heap->free_size.root );
    // check for error or empty
    if ( NULL == address_node ) {
      // extend heap
      extend_heap_space();
      // try another allocate
      return heap_allocate_block( size );
    }
  }

  // get block to split
  block = GET_BLOCK_SIZE( address_node );

  // split flag
  bool split = block->size != size;

  // check for enough size for split
  if ( split && block->size <= real_size ) {
    // extend heap
    extend_heap_space();
    // try another allocate
    return heap_allocate_block( size );
  }

  // remove nodes from free trees
  avl_remove_by_data( &kernel_heap->free_address, block->node_address.data );
  avl_remove_by_node( &kernel_heap->free_size, &block->node_size );

  if ( split ) {
    // calculate remaining size
    uint32_t remaining_size = block->size - real_size;
    uintptr_t new_start = ( uintptr_t )block + real_size;

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "block->size = %d, remaining_size = %d\r\n",
        block->size,
        remaining_size
      )
    #endif

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "block->size = %d\r\n", block->size );
      DEBUG_OUTPUT( "block->address = 0x%08x\r\n", block->address );
    #endif

    // place new block before
    new_block = block;

    // move block
    block = ( heap_block_ptr_t )new_start;
    // prepare block
    prepare_block(
      block,
      ( uintptr_t )block + sizeof( heap_block_t ),
      remaining_size
    );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "block->size = %d\r\n", block->size );
      DEBUG_OUTPUT( "block->address = 0x%08x\r\n", block->address );
    #endif

    // insert nodes at free trees
    avl_insert_by_node( &kernel_heap->free_address, &block->node_address );
    avl_insert_by_node( &kernel_heap->free_size, &block->node_size );
  // found matching node
  } else {
    new_block = block;
  }

  // prepare block
  prepare_block(
    new_block,
    ( uintptr_t )new_block + sizeof( heap_block_t ),
    size
  );

  // insert at used block
  avl_insert_by_node( &kernel_heap->used_area, &new_block->node_address );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
  #endif

  // return address of block
  return new_block->address;
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

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "addr = 0x%08p\r\n", addr );
  #endif

  // finde node by address within tree
  address_node = avl_find_by_data( &kernel_heap->used_area, ( void* )addr );

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
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
  #endif

  // get memory block
  current_block = GET_BLOCK_ADDRESS( address_node );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "current_block = 0x%08x\r\n", current_block );
  #endif

  // remove node from used block
  avl_remove_by_data( &kernel_heap->used_area, address_node->data );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "removed 0x%08x from tree\r\n", &current_block->node_address );

    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
  #endif

  // prepare block
  prepare_block( current_block, current_block->address, current_block->size );

  // insert nodes
  avl_insert_by_node( &kernel_heap->free_address, &current_block->node_address );
  avl_insert_by_node( &kernel_heap->free_size, &current_block->node_size );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
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
      #endif
      // get block structure of sibling
      sibling_block = GET_BLOCK_ADDRESS( sibling_node );
      // merge if possible
      current_block = merge( current_block, sibling_block );
    }

    // right sibling
    sibling_node = current_block->node_address.right;
    // try to merge if not null
    if ( NULL != sibling_node ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "possible sibling for merge = 0x%08x\r\n", sibling_node );
        DEBUG_OUTPUT( "sibling->data = 0x%08x\r\n", sibling_node->data );
      #endif
      // get block structure of sibling
      sibling_block = GET_BLOCK_ADDRESS( sibling_node );
      // merge if possible
      current_block = merge( current_block, sibling_block );
    }

    // find parent
    parent_node = avl_find_parent_by_data(
      &kernel_heap->free_address,
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
      parent_block = GET_BLOCK_ADDRESS( parent_node );
      // merge if possible
      current_block = merge( current_block, parent_block );
    }
  } while ( NULL != parent_node && NULL != current_block );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_address );

    DEBUG_OUTPUT( "%s", "Free size tree:\r\n" );
    avl_print( &kernel_heap->free_size );
  #endif

  // Try to shrink heap if possible
  shrink_heap_space();
}
