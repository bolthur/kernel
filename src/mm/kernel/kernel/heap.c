
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
#include <mm/kernel/kernel/phys.h>
#include <mm/kernel/kernel/virt.h>
#include <mm/kernel/kernel/heap.h>

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
 * @param avl_param parameter
 * @return int32_t
 */
static int32_t compare_address_callback(
  const avl_node_ptr_t avl_a,
  const avl_node_ptr_t avl_b,
  void *avl_param
) {
  // mark parameter as unused
  ( void )avl_param;

  // get block structures
  heap_block_t* a = GET_BLOCK_ADDRESS( avl_a );
  heap_block_t* b = GET_BLOCK_ADDRESS( avl_b );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = 0x%08x, b = 0x%08x\r\n", a, b );
  #endif

  // -1 if address of a is greater than address of b
  if ( a->address > b->address ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( b->address > a->address ) {
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
  kernel_heap->free_area.compare = compare_address_callback;
  kernel_heap->used_area.compare = compare_address_callback;

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
  free_block->node.data = ( vaddr_t )free_block->address;

  // insert into free tree
  avl_insert( &kernel_heap->free_area, &free_block->node );
}

/**
 * @brief Allocate block within heap
 *
 * @param size size to allocate
 * @param alignment address aligment
 * @return vaddr_t address of allocated block
 */
vaddr_t heap_allocate_block( size_t size, size_t alignment ) {
  ( void )size;
  ( void )alignment;
  PANIC( "heap_allocate_block not yet implemented!" );
  return NULL;
}

/**
 * @brief Free block within heap
 *
 * @param addr address to free
 */
void heap_free_block( vaddr_t addr ) {
  // temporary node
  heap_block_t tmp_node;

  // populate for free
  tmp_node.address = addr;
  tmp_node.size = 0;
  tmp_node.node.data = addr;

  // finde node by address within tree
  avl_node_ptr_t node = avl_find( &kernel_heap->used_area, &tmp_node.node );

  // skip if nothing has been found
  if ( NULL == node ) {
    return;
  }

  // remove node from used block
  avl_remove( &kernel_heap->used_area, node );

  // insert into free tree
  avl_insert( &kernel_heap->free_area, node );

  // find parent node
  avl_node_ptr_t parent_node = avl_find_parent(
    &kernel_heap->free_area,
    node
  );

  // no parent node found, skip
  if ( NULL == parent_node ) {
    return;
  }

  // get parent and current block
  heap_block_ptr_t parent_block = GET_BLOCK_ADDRESS( parent_node );
  heap_block_ptr_t current_block = GET_BLOCK_ADDRESS( node );

  // determine end of parent and begin of current block
  vaddr_t end = ( vaddr_t )( ( paddr_t )&parent_block + parent_block->size );
  vaddr_t begin = ( vaddr_t )( ( paddr_t )&current_block );

  // merge on match
  if ( begin == end ) {
    // increase
    parent_block->size += current_block->size;

    // remove block again
    avl_remove( &kernel_heap->free_area, node );
  }
}
