
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
#include <avl/avl.h>
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
 * @param avl_a node a
 * @param avl_b node b
 * @return int32_t
 */
static int32_t compare_address_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = 0x%08x, b = 0x%08x\r\n", a, b );
    DEBUG_OUTPUT( "a->data = 0x%08x, b->data = 0x%08x\r\n", a->data, b->data );
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
 * @param avl_a node a
 * @param avl_b node b
 * @return int32_t
 */
static int32_t compare_size_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = 0x%08x, b = 0x%08x\r\n", a, b );
    DEBUG_OUTPUT( "a->data = 0x%08x, b->data = 0x%08x\r\n", a->data, b->data );
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
  vaddr_t addr,
  size_t size
) {
  // clear memory
  memset( ( vaddr_t )block, 0, size + sizeof( heap_block_t ) );

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
static void extend_heap( void ) {
  avl_node_ptr_t max, max_used, max_free;
  heap_block_ptr_t block, free_block;
  paddr_t heap_end;
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
  heap_end = ( paddr_t )block->address + block->size;
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
    vaddr_t addr = ( vaddr_t )heap_end;
    addr < ( vaddr_t )( ( paddr_t )heap_end + HEAP_EXTENSION );
    addr = ( vaddr_t )( ( paddr_t )addr + PAGE_SIZE )
  ) {
    // Find free page
    paddr_t phys = ( paddr_t )phys_find_free_page( PAGE_SIZE );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map 0x%08x to 0x%08x\r\n", phys, addr );
    #endif

    // map address
    virt_map_address(
      kernel_context, addr, phys, PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );
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
    free_block->address = ( vaddr_t )(
      ( paddr_t )free_block + sizeof( heap_block_t )
    );
    free_block->size = HEAP_EXTENSION;
  }

  // populate node data
  avl_prepare_node( &free_block->node_address, free_block->address );
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
 *
 * @todo add logic for shrink of heap
 */
static void shrink_heap( void ) {
}

/**
 * @brief Initialize heap
 */
void heap_init( void ) {
  // assert not initialized
  assert( NULL == kernel_heap );

  // offset for first free block
  uint32_t offset = 0;
  vaddr_t start = ( vaddr_t )HEAP_START;

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
    vaddr_t addr = start;
    addr < ( vaddr_t )( ( paddr_t )HEAP_START + HEAP_MIN_SIZE );
    addr = ( vaddr_t )( ( paddr_t )addr + PAGE_SIZE )
  ) {
    // Find free page
    paddr_t phys = ( paddr_t )phys_find_free_page( PAGE_SIZE );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map 0x%08x to 0x%08x\r\n", phys, addr );
    #endif

    // map address
    virt_map_address(
      kernel_context, addr, phys, PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "heap start: 0x%08x\r\n", HEAP_START );
  #endif

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
  memset( ( vaddr_t )kernel_heap, 0, sizeof( heap_manager_t ) );

  // populate trees
  kernel_heap->free_address.compare = compare_address_callback;
  kernel_heap->used_area.compare = compare_address_callback;
  kernel_heap->free_size.compare = compare_size_callback;

  // create free block
  heap_block_ptr_t free_block = ( heap_block_ptr_t )(
    ( paddr_t )HEAP_START + offset
  );

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
  prepare_block(
    free_block,
    ( vaddr_t )( ( paddr_t )HEAP_START + offset ),
    HEAP_MIN_SIZE - offset
  );

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
 * @param alignment address aligment
 * @return vaddr_t address of allocated block
 */
vaddr_t heap_allocate_block( size_t size ) {
  // variables
  heap_block_ptr_t block, new_block;
  avl_node_ptr_t address_node;
  size_t real_size;

  // calculate real size
  real_size = size + sizeof( heap_block_t );
  uint32_t remaining_size;
  paddr_t new_start;

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
      extend_heap();
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
    PANIC( "Block to split is to small, need to extend heap!" );
  }

  // remove nodes from free trees
  avl_remove_by_data( &kernel_heap->free_address, block->node_address.data );
  avl_remove_by_node( &kernel_heap->free_size, &block->node_size );

  if ( split ) {
    // calculate remaining size
    remaining_size = block->size - real_size;
    new_start = ( paddr_t )block + real_size;

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
      ( vaddr_t )( ( paddr_t )block + sizeof( heap_block_t ) ),
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
    ( vaddr_t )( ( paddr_t )new_block + sizeof( heap_block_t ) ),
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
 *
 * @todo Merge possible free blocks
 */
void heap_free_block( vaddr_t addr ) {
  // variables
  avl_node_ptr_t address_node;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "addr = 0x%08x\r\n", addr );
  #endif

  // finde node by address within tree
  address_node = avl_find_by_data( &kernel_heap->used_area, addr );

  // skip if nothing has been found
  if ( NULL == address_node ) {
    return;
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "address_node = 0x%08x, data = 0x%08x\r\n",
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
  heap_block_ptr_t current_block = GET_BLOCK_ADDRESS( address_node );

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

  return;

  // FIXME: Get and check parent for merge
  // FIXME: Check left sibling for merge
  // FIXME: Check right sibling for merge

  // find parent
  avl_node_ptr_t parent = avl_find_parent_by_data(
    &kernel_heap->free_address,
    current_block->node_address.data
  );

  // no parent found?
  if ( NULL == parent ) {
    return;
  }

  // loop until no possible merge parent
  do {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "possible parent for merge = 0x%08x\r\n", parent );
      DEBUG_OUTPUT( "parent->data = 0x%08x\r\n", parent->data );
    #endif

    heap_block_ptr_t parent_block = GET_BLOCK_ADDRESS( parent );

    paddr_t parent_begin = ( paddr_t )parent_block;
    paddr_t current_end = ( paddr_t )current_block->address + current_block->size;
    paddr_t current_begin = ( paddr_t )current_block;
    paddr_t parent_end = ( paddr_t )parent_block->address + parent_block->size;

    // debug output
    DEBUG_OUTPUT(
      "current_begin = 0x%08x, parent_end = 0x%08x\r\n",
      current_begin,
      parent_end
    );
    DEBUG_OUTPUT(
      "parent_begin = 0x%08x, current_end = 0x%08x\r\n",
      parent_begin,
      current_end
    );

    // extend parent
    if ( current_end == parent_begin ) {
      // remove node from free tree
      avl_remove_by_data( &kernel_heap->free_address, parent->data );
      avl_remove_by_data( &kernel_heap->free_address, current_block->node_address.data );
      // remove node from free size
      avl_remove_by_node( &kernel_heap->free_size, &parent_block->node_size );
      avl_remove_by_node( &kernel_heap->free_size, &current_block->node_size );
      // extend current block
      current_block->size += parent_block->size;
      // prepare nodes
      avl_prepare_node( &current_block->node_address, current_block->address );
      avl_prepare_node( &current_block->node_size, ( void* )current_block->size );
      // insert again
      avl_insert_by_node( &kernel_heap->free_address, &current_block->node_address );
      avl_insert_by_node( &kernel_heap->free_size, &current_block->node_size );

      // overwrite parent
      parent = &current_block->node_address;
    }

    if ( current_begin == parent_end ) {
       // remove node from free tree
      avl_remove_by_data( &kernel_heap->free_address, parent->data );
      avl_remove_by_data( &kernel_heap->free_address, current_block->node_address.data );
      // remove node from free size
      avl_remove_by_node( &kernel_heap->free_size, &parent_block->node_size );
      avl_remove_by_node( &kernel_heap->free_size, &current_block->node_size );
      // extend current block
      parent_block->size += current_block->size;
      // prepare nodes
      avl_prepare_node( &current_block->node_address, current_block->address );
      avl_prepare_node( &current_block->node_size, ( void* )current_block->size );
      // insert again
      avl_insert_by_node( &kernel_heap->free_address, &current_block->node_address );
      avl_insert_by_node( &kernel_heap->free_size, &current_block->node_size );
      // overwrite current
      current_block = parent_block;
    }

    // find parent of parent
    parent = avl_find_parent_by_data( &kernel_heap->free_address, parent->data );
  } while ( NULL != parent );

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
  shrink_heap();
}
