
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
 * @brief Getter for heap initialized flag
 *
 * @return true
 * @return false
 */
bool heap_initialized_get( void ) {
  return NULL != kernel_heap;
}

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
  free_block->address = ( vaddr_t )(
    ( paddr_t )HEAP_START + offset
  );
  free_block->size = HEAP_MIN_SIZE - offset;
  // populate node data
  avl_prepare_node( &free_block->node_address, free_block->address );
  avl_prepare_node( &free_block->node_size, ( void* )free_block->size );

  // insert into free tree
  avl_insert( &kernel_heap->free_address, &free_block->node_address );
  avl_insert( &kernel_heap->free_size, &free_block->node_size );

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

  // Try to find one that matches by size
  address_node = avl_find(
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
      PANIC( "Size tree is empty, need to extend heap!" );
    }
  }

  // get block to split
  block = GET_BLOCK_SIZE( address_node );

  // split flag
  bool split = block->size != size;

  // check for enough size for split
  if ( split && block->size <= real_size ) {
    // FIXME: Extend heap here
    PANIC( "Block to split is to small, need to extend heap!" );
  }

  // remove nodes from free trees
  avl_remove( &kernel_heap->free_address, block->node_address.data );
  avl_remove_by_node( &kernel_heap->free_size, &block->node_size );

  if ( split ) {
    // calculate remaining size
    remaining_size = block->size - real_size;
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "block->size = %d, remaining_size = %d\r\n",
        block->size,
        remaining_size
      )
    #endif
    // update block
    block->size = remaining_size;
    // refresh node size data
    avl_prepare_node( &block->node_size, ( void* )block->size );
    avl_prepare_node( &block->node_address, ( void* )block->address );

    // insert nodes from free trees
    avl_insert( &kernel_heap->free_address, &block->node_address );
    avl_insert( &kernel_heap->free_size, &block->node_size );

    // place new block
    new_block = ( heap_block_ptr_t )(
      ( paddr_t )block->address + remaining_size
    );
  // found matching node
  } else {
    new_block = block;
  }

  // clear new block
  memset( ( vaddr_t )new_block, 0, real_size );
  // setup new block
  new_block->size = size;
  new_block->address = ( vaddr_t )(
    ( paddr_t )new_block + sizeof( heap_block_t )
  );
  // prepare nodes
  avl_prepare_node( &new_block->node_address, new_block->address );
  avl_prepare_node( &new_block->node_size, ( void* )new_block->size );
  // insert at used block
  avl_insert( &kernel_heap->used_area, &new_block->node_address );

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
  address_node = avl_find( &kernel_heap->used_area, addr );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "address_node = 0x%08x, data = 0x%08x\r\n",
      address_node,
      address_node->data
    );
    DEBUG_OUTPUT(
      "used_tree root data: 0x%08x\r\n", kernel_heap->used_area.root->data
    );
  #endif

  // skip if nothing has been found
  if ( NULL == address_node ) {
    return;
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

  // get memory block
  heap_block_ptr_t current_block = GET_BLOCK_ADDRESS( address_node );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "current_block = 0x%08x\r\n", current_block );
  #endif

  // remove node from used block
  avl_remove( &kernel_heap->used_area, address_node->data );

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

  // prepare nodes again
  avl_prepare_node( &current_block->node_address, current_block->address );
  avl_prepare_node( &current_block->node_size, ( void* )current_block->size );
  // insert nodes
  avl_insert( &kernel_heap->free_address, &current_block->node_address );
  avl_insert( &kernel_heap->free_size, &current_block->node_size );

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

  // find parent
  avl_node_ptr_t parent = avl_find_parent(
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
      avl_remove( &kernel_heap->free_address, parent->data );
      avl_remove( &kernel_heap->free_address, current_block->node_address.data );
      // remove node from free size
      avl_remove_by_node( &kernel_heap->free_size, &parent_block->node_size );
      avl_remove_by_node( &kernel_heap->free_size, &current_block->node_size );
      // extend current block
      current_block->size += parent_block->size;
      // prepare nodes
      avl_prepare_node( &current_block->node_address, current_block->address );
      avl_prepare_node( &current_block->node_size, ( void* )current_block->size );
      // insert again
      avl_insert( &kernel_heap->free_address, &current_block->node_address );
      avl_insert( &kernel_heap->free_size, &current_block->node_size );

      // overwrite parent
      parent = &current_block->node_address;
    }

    if ( current_begin == parent_end ) {
       // remove node from free tree
      avl_remove( &kernel_heap->free_address, parent->data );
      avl_remove( &kernel_heap->free_address, current_block->node_address.data );
      // remove node from free size
      avl_remove_by_node( &kernel_heap->free_size, &parent_block->node_size );
      avl_remove_by_node( &kernel_heap->free_size, &current_block->node_size );
      // extend current block
      parent_block->size += current_block->size;
      // prepare nodes
      avl_prepare_node( &current_block->node_address, current_block->address );
      avl_prepare_node( &current_block->node_size, ( void* )current_block->size );
      // insert again
      avl_insert( &kernel_heap->free_address, &current_block->node_address );
      avl_insert( &kernel_heap->free_size, &current_block->node_size );
      // overwrite current
      current_block = parent_block;
    }

    // find parent of parent
    parent = avl_find_parent( &kernel_heap->free_address, parent->data );
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
}
