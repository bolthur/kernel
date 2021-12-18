/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <collection/avl.h>
#if defined( PRINT_MM_HEAP )
  #include <debug/debug.h>
#endif
#include <mm/phys.h>
#include <mm/virt.h>
#include <mm/heap.h>
#include <panic.h>

/**
 * @brief Kernel heap
 */
heap_manager_ptr_t kernel_heap = NULL;

/**
 * @fn avl_tree_ptr_t get_free_address_tree(heap_init_state_t, heap_manager_ptr_t)
 * @brief Get the free address tree object
 *
 * @param state
 * @param heap
 * @return
 */
static avl_tree_ptr_t get_free_address_tree(
  heap_init_state_t state,
  heap_manager_ptr_t heap
) {
  if ( ! heap ) {
    return NULL;
  }

  return &heap->free_address[ state ];
}

/**
 * @fn avl_tree_ptr_t get_free_size_tree(heap_init_state_t, heap_manager_ptr_t)
 * @brief Get the free size tree object
 *
 * @param state
 * @param heap
 * @return
 */
static avl_tree_ptr_t get_free_size_tree(
  heap_init_state_t state,
  heap_manager_ptr_t heap
) {
  if ( ! heap ) {
    return NULL;
  }

  return &heap->free_size[ state ];
}

/**
 * @fn avl_tree_ptr_t get_used_area_tree(heap_init_state_t, heap_manager_ptr_t)
 * @brief Get the used area tree object
 *
 * @param state
 * @param heap
 * @return
 */
static avl_tree_ptr_t get_used_area_tree(
  heap_init_state_t state,
  heap_manager_ptr_t heap
) {
  if ( ! heap ) {
    return NULL;
  }

  return &heap->used_area[ state ];
}

/**
 * @fn int32_t compare_address_callback(const avl_node_ptr_t, const avl_node_ptr_t)
 * @brief Compare address callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return
 */
static int32_t compare_address_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %p, b->data = %p\r\n", a->data, b->data );
  #endif

  // -1 if address of a->data is greater than address of b->data
  if ( a->data > b->data ) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
  } else if ( b->data > a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @fn int32_t compare_size_callback(const avl_node_ptr_t, const avl_node_ptr_t)
 * @brief Compare address callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return
 */
static int32_t compare_size_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %p, b->data = %p\r\n", a->data, b->data );
  #endif

  // -1 if address of a->data is greater than address of b->data
  if ( ( size_t )a->data > ( size_t )b->data ) {
    return -1;
  // 1 if address of b->data is greater than address of a->data
  } else if ( ( size_t )b->data > ( size_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @fn void prepare_block(heap_block_ptr_t, uintptr_t, size_t)
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
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "block = %p, addr = %p, size = %zu\r\n",
      ( void* )block, ( void* )addr, size );
  #endif

  // clear memory
  memset( ( void* )block, 0, size + sizeof( heap_block_t ) );

  // prepare block itself
  block->size = size;
  block->address = addr;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "block->node_address = %p, block->node_size = %p\r\n",
      &block->node_address, &block->node_size );
  #endif

  // prepare tree nodes
  avl_prepare_node( &block->node_address, ( void* )addr );
  avl_prepare_node( &block->node_size, ( void* )size );
}

/**
 * @fn bool extend_heap_space(size_t)
 * @brief Helper to extend heap size
 *
 * @param given_size size to extend by at minimum
 * @return
 */
static bool extend_heap_space( size_t given_size ) {
  avl_node_ptr_t max;
  avl_node_ptr_t max_used;
  avl_node_ptr_t max_free;
  heap_block_ptr_t block;
  heap_block_ptr_t free_block;
  uintptr_t heap_end;
  size_t heap_size;
  avl_tree_ptr_t used_area;
  avl_tree_ptr_t free_address;
  avl_tree_ptr_t free_size;

  // stop if not setup
  if ( ! kernel_heap || HEAP_INIT_NORMAL != kernel_heap->state ) {
    return false;
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
  if ( max_used && max_free ) {
    if ( max_free->data > max_used->data ) {
      max = max_free;
    } else {
      max = max_used;
    }
  } else if ( max_used ) {
    max = max_used;
  } else if ( max_free ) {
    max = max_free;
  }

  // check max
  if ( ! max ) {
    return false;
  }

  // get block
  block = HEAP_GET_BLOCK_ADDRESS( max );
  // get heap end and current size
  heap_end = block->address + block->size;
  heap_size = heap_end - HEAP_START;
  size_t real_extension_size = 0;
  while ( real_extension_size < given_size ) {
    real_extension_size += HEAP_EXTENSION;
  }
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "End: %p, Size: %zx, new end: %p, New size: %zx\r\n",
      ( void* )heap_end,
      heap_size,
      ( void* )( heap_end + heap_size + real_extension_size ),
      heap_size + real_extension_size
    );
  #endif
  // check size against max size
  if ( heap_size + real_extension_size >= HEAP_MAX_SIZE ) {
    return false;
  }

  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "extend_heap_space( %#x )\r\n", real_extension_size );
  #endif

  // map heap address space
  for (
    uintptr_t addr = heap_end;
    addr < heap_end + real_extension_size;
    addr += PAGE_SIZE
  ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map %p with random physical address\r\n", ( void* )addr );
    #endif

    // map address
    if ( ! virt_map_address_random(
      virt_current_kernel_context,
      addr,
      VIRT_MEMORY_TYPE_NORMAL_NC,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) ) {
      return false;
    }
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
    free_block->size += real_extension_size;
  // Create total new block
  } else {
    // create free block
    free_block = ( heap_block_ptr_t )heap_end;

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Created free block at %p\r\n", ( void* )free_block );
    #endif

    // prepare free block
    free_block->address = ( uintptr_t )free_block + sizeof( heap_block_t );
    free_block->size = real_extension_size - sizeof( heap_block_t );
  }
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "free_block = %p, free_block->address = %#"PRIxPTR
      ", free_block->size = %ux\r\n",
      ( void* )free_block,
      free_block->address,
      free_block->size
    )
  #endif

  // populate node data
  avl_prepare_node( &free_block->node_address, ( void* )free_block->address );
  avl_prepare_node( &free_block->node_size, ( void* )free_block->size );

  // insert into free trees
  assert(
    avl_insert_by_node( free_address, &free_block->node_address )
    && avl_insert_by_node( free_size, &free_block->node_size )
  )

  // increase size
  kernel_heap->size += real_extension_size;

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  return true;
}

/**
 * @fn void shrink_heap_space(void)
 * @brief Helper to shrink heap if possible
 */
static void shrink_heap_space( void ) {
  heap_block_ptr_t max_free_block;
  heap_block_ptr_t max_used_block = NULL;
  size_t max_end;
  size_t shrink_size;
  avl_node_ptr_t max_free_node;
  avl_node_ptr_t max_used_node;
  avl_tree_ptr_t free_address;
  avl_tree_ptr_t free_size;
  avl_tree_ptr_t used_address;
  size_t old_size;
  bool early_exit;
  size_t to_remove = 0;

  // stop if not setup
  if (
    ! kernel_heap
    || HEAP_INIT_NORMAL != kernel_heap->state
  ) {
    return;
  }

  // get correct trees
  free_address = get_free_address_tree( kernel_heap->state, kernel_heap );
  free_size = get_free_size_tree( kernel_heap->state, kernel_heap );
  used_address = get_used_area_tree( kernel_heap->state, kernel_heap );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Try to shrink heap!\r\n" );
  #endif

  // get max used node
  max_used_node = avl_get_max( used_address->root );
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_node = %p\r\n", ( void* )max_used_node );
  #endif
  // set max used block if something is there
  if ( max_used_node ) {
    max_used_block = HEAP_GET_BLOCK_ADDRESS( max_used_node );
  }

  // get node on right side
  max_free_node = avl_get_max( free_address->root );
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_free_node = %p\r\n", ( void* )max_free_node );
  #endif
  // skip if there is nothing
  if ( ! max_free_node ) {
    return;
  }

  // get block
  max_free_block = HEAP_GET_BLOCK_ADDRESS( max_free_node );
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_block = %p\r\n", ( void* )max_free_block );
  #endif
  // skip shrink if used block is greater than free one
  if ( max_used_block && max_used_block->address > max_free_block->address ) {
    // Debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Max free block is in the middle\r\n" );
    #endif
    // skip shrink
    return;
  }

  // determine end of block
  max_end = ( size_t )max_free_block ->address + max_free_block ->size;
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "max_end = %p\r\n", ( void* )max_end );
  #endif
  // do nothing if not yet expanded
  if ( HEAP_START + HEAP_MIN_SIZE >= max_end ) {
    return;
  }

  // determine shrink size
  shrink_size = max_end - ( HEAP_START + HEAP_MIN_SIZE );
  // do nothing if expand size is below expand unit
  if (
    HEAP_EXTENSION > shrink_size
    || max_free_block->size < HEAP_EXTENSION
  ) {
    return;
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "shrink_heap_space()\r\n" );
    DEBUG_OUTPUT( "max_block->size = %"PRIxPTR"\r\n", max_free_block->size );
    DEBUG_OUTPUT( "shrink_size = %p\r\n", ( void* )shrink_size );
  #endif

  // remove from free address
  avl_remove_by_data( free_address, max_free_block->node_address.data );
  // remove from free size tree
  avl_remove_by_node( free_size, &max_free_block->node_size );

  old_size = max_free_block->size;
  // adjust block size
  while ( max_free_block->size >= HEAP_EXTENSION && shrink_size > 0 ) {
    // debug output stat only
    to_remove += HEAP_EXTENSION;
    max_free_block->size -= HEAP_EXTENSION;
    shrink_size -= HEAP_EXTENSION;
  }
  // stop
  early_exit = 0 == max_free_block->size;
  if ( 0 == max_free_block->size ) {
    max_free_block->size = old_size;
  }
  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "to_remove = %p\r\n", ( void* )to_remove );
  #endif
  // prepare blocks after adjustment
  prepare_block(
    max_free_block,
    max_free_block->address,
    max_free_block->size
  );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "max_free_block = %p, max_free_block->address = %#"PRIxPTR
      ", max_free_block->size = %ux\r\n",
      ( void* )max_free_block,
      max_free_block->address,
      max_free_block->size
    )
  #endif
  // reinsert block with assertion
  assert(
    avl_insert_by_node( free_address, &max_free_block->node_address )
    && avl_insert_by_node( free_size, &max_free_block->node_size )
  )
  // early exit
  if ( early_exit ) {
    return;
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "unmap = %"PRIxPTR", size = %"PRIxPTR", max_end = %"PRIxPTR"\r\n",
      max_free_block->address,
      max_free_block->size,
      max_end
    );
  #endif
  // free up virtual memory
  for (
    uintptr_t start = max_free_block->address + max_free_block->size;
    start < max_end;
    start += PAGE_SIZE
  ) {
    // Debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "unmap = %"PRIxPTR"\r\n", start );
    #endif
    // unmap virtual address
    virt_unmap_address( virt_current_kernel_context, start, true );
  }
  // increase size
  kernel_heap->size -= to_remove;

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_address );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif
}

/**
 * @fn bool combinable(heap_block_ptr_t, heap_block_ptr_t)
 * @brief Helper to check whether merge of two passed blocks is possible
 *
 * @param a block a
 * @param b block b
 * @return
 */
static bool combinable( heap_block_ptr_t a, heap_block_ptr_t b ) {
  uintptr_t a_end;
  uintptr_t b_end;

  // calculate end of a and b
  a_end = a->address + a->size;
  b_end = b->address + b->size;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "a = %p, a->address = %p, size = %zu, a_end = %p\r\n",
      ( void* )a, ( void* )a->address, a->size, ( void* )a_end
    );
    DEBUG_OUTPUT(
      "b = %p, b->address = %p, size = %zu, b_end = %p\r\n",
      ( void* )b, ( void* )b->address, b->size, ( void* )b_end
    );
  #endif

  // combinable if following directly
  return a_end == ( uintptr_t )b || b_end == ( uintptr_t )a;
}

/**
 * @fn heap_block_ptr_t merge(heap_block_ptr_t, heap_block_ptr_t)
 * @brief Method merges two blocks if possible
 *
 * @param a block a
 * @param b block b
 * @return
 */
static heap_block_ptr_t merge( heap_block_ptr_t a, heap_block_ptr_t b ) {
  uintptr_t a_end;
  uintptr_t b_end;
  heap_block_ptr_t to_insert = NULL;
  avl_tree_ptr_t free_address;
  avl_tree_ptr_t free_size;

  // skip if not combinable
  if ( true != combinable( a, b ) ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "a ( %p ) not combinable with b ( %p )\r\n",
        ( void* )a, ( void* )b );
    #endif
    return NULL;
  }

  // calculate end of a and b
  a_end = a->address + a->size;
  b_end = b->address + b->size;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "a =  %p, a->address =  %p, size = %zu, a_end =  %p\r\n",
      ( void* )a, ( void* )a->address, a->size, ( void* )a_end
    );
    DEBUG_OUTPUT(
      "b =  %p, b->address =  %p, size = %zu, b_end =  %p\r\n",
      ( void* )b, ( void* )b->address, b->size, ( void* )b_end
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
  } else {
    return NULL;
  }

  // prepare blocks of element to insert
  prepare_block( to_insert, to_insert->address, to_insert->size );
  // insert merged node
  assert(
    avl_insert_by_node( free_address, &to_insert->node_address )
    && avl_insert_by_node( free_size, &to_insert->node_size )
  )
  // return pointer to merged node
  return to_insert;
}

/**
 * @fn bool merge_address_tree(avl_node_ptr_t)
 * @brief Iterate address tree and merge if possible
 *
 * @param root
 * @return
 */
static bool merge_address_tree( avl_node_ptr_t root ) {
  // handle empty tree
  if ( ! root ) {
    return false;
  }
  // get current block
  heap_block_ptr_t current_block = HEAP_GET_BLOCK_ADDRESS( root );
  // try to merge if not null
  if ( root->left ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "possible sibling for merge = %p\r\n", ( void* )root->left );
      DEBUG_OUTPUT( "sibling->data = %p\r\n", ( void* )root->left->data );
      DEBUG_OUTPUT( "current_block = %p\r\n", ( void* )current_block );
    #endif
    // handle return
    if ( merge( current_block, HEAP_GET_BLOCK_ADDRESS( root->left ) ) ) {
      return true;
    }
  }
  // try to merge if not null
  if ( root->right ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "possible sibling for merge = %p\r\n", ( void* )root->right );
      DEBUG_OUTPUT( "sibling->data = %p\r\n", root->right->data );
      DEBUG_OUTPUT( "current_block = %p\r\n", ( void* )current_block );
      DEBUG_OUTPUT( "root = %p\r\n", ( void* )root );
    #endif
    // handle return
    if ( merge( current_block, HEAP_GET_BLOCK_ADDRESS( root->right ) ) ) {
      return true;
    }
  }
  // continue with left tree
  if ( merge_address_tree( root->left ) ) {
    return true;
  }
  // continue with right tree
  if ( merge_address_tree( root->right ) ) {
    return true;
  }
  // nothing remaining to merge
  return false;
}

/**
 * @fn void heap_init(heap_init_state_t)
 * @brief Initialize heap
 *
 * @param state
 */
void heap_init( heap_init_state_t state ) {
  // correct state
  // skip if wrong state is passed
  if (
    (
      HEAP_INIT_EARLY != state
      && HEAP_INIT_NORMAL != state
    ) || (
      kernel_heap
      && kernel_heap->state > state
    )
  ) {
    return;
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
      "Heap start: %p, Heap end: %p\r\n",
      ( void* )start,
      ( void* )( start + min_size )
    );
    DEBUG_OUTPUT( "state = %d\r\n", state );
  #endif

  // map heap address space
  if ( HEAP_INIT_NORMAL == state ) {
    // set size
    kernel_heap->size = min_size;
    for (
      uintptr_t addr = start;
      addr < start + min_size;
      addr += PAGE_SIZE
    ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "Map %p with random physical address\r\n", ( void* )addr );
      #endif

      // map address
      assert( virt_map_address_random(
        virt_current_kernel_context,
        addr,
        VIRT_MEMORY_TYPE_NORMAL_NC,
        VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
      ) )
    }
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Clearing out heap area\r\n" );
  #endif
  // erase kernel heap section
  memset( ( void* )start, 0, min_size );

  // set kernel heap and increase offset
  if ( ! kernel_heap ) {
    heap = ( heap_manager_ptr_t )start;
    offset += sizeof( heap_manager_t );
  } else {
    heap = kernel_heap;
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Placed heap management at %p\r\n", ( void* )heap );
    DEBUG_OUTPUT( "offset: %u\r\n", offset );
  #endif

  // initialize management structure
  if ( ! kernel_heap ) {
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
  heap->free_address[ state ].lookup = avl_default_lookup;
  heap->free_address[ state ].cleanup = avl_default_cleanup;

  heap->used_area[ state ].compare = compare_address_callback;
  heap->used_area[ state ].lookup = avl_default_lookup;
  heap->used_area[ state ].cleanup = avl_default_cleanup;

  heap->free_size[ state ].compare = compare_size_callback;
  heap->free_size[ state ].lookup = avl_default_lookup;
  heap->free_size[ state ].cleanup = avl_default_cleanup;

  // create free block
  heap_block_ptr_t free_block = ( heap_block_ptr_t )( start + offset );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Placing free block at %p\r\n", ( void* )free_block );
  #endif

  // increase offset
  offset += sizeof( heap_block_t );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Heap offset: %u\r\n", offset );
    DEBUG_OUTPUT( "Preparing and inserting free block\r\n" );
  #endif

  // prepare free block
  prepare_block( free_block, start + offset, min_size - offset );
  // insert into free trees
  assert(
    avl_insert_by_node( &heap->free_address[ state ], &free_block->node_address )
    && avl_insert_by_node( &heap->free_size[ state ], &free_block->node_size )
  )

  // finally, set state
  heap->state = state;
  // set kernel heap global if null
  if ( ! kernel_heap ) {
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
 * @fn bool heap_init_get(void)
 * @brief Getter for heap initialized flag
 *
 * @return
 */
bool heap_init_get( void ) {
  return ( bool )kernel_heap;
}

/**
 * @fn uintptr_t heap_allocate_block(size_t, size_t)
 * @brief Allocate block within heap
 *
 * @param alignment memory alignment
 * @param size size to allocate
 * @return address of allocated block
 */
uintptr_t heap_allocate_block( size_t alignment, size_t size ) {
  // variables
  heap_block_ptr_t current;
  heap_block_ptr_t new;
  heap_block_ptr_t following;
  heap_block_ptr_t previous;
  avl_node_ptr_t address_node;
  size_t real_size;
  size_t alignment_offset;
  bool split;
  avl_tree_ptr_t used_area;
  avl_tree_ptr_t free_address;
  avl_tree_ptr_t free_size;

  // stop if not setup
  if ( ! kernel_heap ) {
    return 0;
  }
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "alignment = %zx, size = %zu\r\n", alignment, size );
  #endif

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
    DEBUG_OUTPUT( "address_node = %p\r\n", ( void* )address_node );
  #endif
  // check alignment for possible matching block
  if ( address_node ) {
    // get block pointer
    current = HEAP_GET_BLOCK_SIZE( address_node );

    // check for not matching alignment
    if ( current->address % alignment ) {
      // set address node to null on alignment mismatch
      address_node = NULL;
    }
  }
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "address_node = %p\r\n", ( void* )address_node );
  #endif

  // Check for no matching node has been found
  if ( ! address_node ) {
    // get max node
    address_node = avl_get_max( free_size->root );
    // check for error or empty
    if ( ! address_node ) {
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
      if ( ! extend_heap_space( real_size ) ) {
        return 0;
      }
      // try another allocation
      return heap_allocate_block( alignment, size );
    }
  }

  // get block to split
  current = HEAP_GET_BLOCK_SIZE( address_node );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "address_node = %p\r\n", ( void* )address_node );
    DEBUG_OUTPUT( "current->size = %zx\r\n", current->size );
    DEBUG_OUTPUT( "current->address = %p\r\n", ( void* )current->address );
  #endif

  // split flag
  split = current->size != size;
  // check for enough size for split
  if ( split && current->size <= real_size ) {
    // no expansion within early heap state
    if ( HEAP_INIT_EARLY == kernel_heap->state ) {
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "Not enough space, existing size: %zu, real size = %zu\r\n",
          current->size,
          real_size
        );
      #endif
      return 0;
    }
    // extend heap
    if ( ! extend_heap_space( real_size ) ) {
      return 0;
    }
    // try another allocation
    return heap_allocate_block( alignment, size );
  }

  // possible alignment offset
  alignment_offset = 0;
  // handle alignment
  if ( current->address % alignment ) {
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "alignment_offset = %zu\r\n", alignment_offset );
    #endif

    // determine alignment offset
    alignment_offset = ( alignment - ( current->address % alignment ) );
    do {
      alignment_offset += alignment;
    } while ( alignment_offset < real_size );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "alignment_offset = %zu\r\n", alignment_offset );
    #endif

    // subtract heap block size
    alignment_offset -= sizeof( heap_block_t );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "alignment_offset = %zu\r\n", alignment_offset );
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
      if ( ! extend_heap_space( real_size ) ) {
        return 0;
      }
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
    uint32_t block_alignment = new_start % sizeof( heap_block_t );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "block->size = %zu, remaining_size = %u, new_start %p\r\n",
        current->size, remaining_size, ( void* )new_start );
    #endif
    // handle proper block structure alignment
    if ( block_alignment ) {
      // calculate correct alignment
      block_alignment = sizeof( heap_block_t )
        - block_alignment % sizeof( heap_block_t );
      // debug output
      #if defined( PRINT_MM_HEAP )
        DEBUG_OUTPUT( "added offset for heap block = %p\r\n",
          ( void* )block_alignment );
      #endif
      // increment new start and decrement remaining size
      new_start += block_alignment;
      remaining_size -= block_alignment;
      // adjust size of block
      size += block_alignment;
    }

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "block->size = %zu, remaining_size = %u, new_start %p\r\n",
        current->size, remaining_size, ( void* )new_start );
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
      DEBUG_OUTPUT( "block->size = %zu\r\n", current->size );
      DEBUG_OUTPUT( "block->address = %p\r\n", ( void* )current->address );
    #endif

    // insert nodes at free trees
    assert(
      avl_insert_by_node( free_address, &current->node_address )
      && avl_insert_by_node( free_size, &current->node_size )
    )
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
      DEBUG_OUTPUT( "check_following_size = %zu\r\n", check_following_size );
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
        DEBUG_OUTPUT( "following->size = %zu\r\n", following->size );
        DEBUG_OUTPUT( "following->address = %p\r\n", ( void* )following->address );
      #endif

      // insert nodes at free trees
      assert(
        avl_insert_by_node( free_address, &following->node_address )
        && avl_insert_by_node( free_size, &following->node_size )
      )
    }

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "previous->size = %zu\r\n", previous->size );
      DEBUG_OUTPUT( "previous->address = %p\r\n", ( void* )previous->address );
    #endif

    // prepare previous block
    prepare_block(
      previous, previous->address, ( uintptr_t )new - previous->address );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "previous->size = %zu\r\n", previous->size );
      DEBUG_OUTPUT( "previous->address = %p\r\n", ( void* )previous->address );
    #endif

    // insert nodes at free trees
    assert(
      avl_insert_by_node( free_address, &previous->node_address )
      && avl_insert_by_node( free_size, &previous->node_size )
    )
  // found matching node
  } else {
    new = current;
  }

  // prepare block
  prepare_block( new, ( uintptr_t )new + sizeof( heap_block_t ), size );
  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "new->size = %zu\r\n", new->size );
    DEBUG_OUTPUT( "new->address = %p\r\n", ( void* )new->address );
  #endif

  // insert at used block
  assert( avl_insert_by_node( used_area, &new->node_address ) )

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
 * @fn void heap_free_block(uintptr_t)
 * @brief Free block within heap
 *
 * @param addr address to free
 */
void heap_free_block( uintptr_t addr ) {
  // variables
  avl_node_ptr_t address_node;
  heap_block_ptr_t current_block;
  avl_tree_ptr_t used_area;
  avl_tree_ptr_t free_address;
  avl_tree_ptr_t free_size;

  // stop if not setup
  if ( ! kernel_heap ) {
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
    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT(
        "HEAP_START = %#"PRIxPTR", HEAP_START + kernel_heap->size = %#"PRIxPTR
        ", addr = %#"PRIxPTR"\r\n",
        HEAP_START,
        HEAP_START + kernel_heap->size,
        addr
      )
    #endif
    // assert address is within valid range
    if ( addr < HEAP_START || addr > HEAP_START + kernel_heap->size ) {
      PANIC( "Invalid address free tried!" )
    }
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "addr = %p\r\n", ( void* )addr );
  #endif

  // find node by address within tree
  address_node = avl_find_by_data( used_area, ( void* )addr );

  // skip if nothing has been found
  if ( ! address_node ) {
    return;
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "address_node = %p, data = %p\r\n",
      ( void* )address_node,
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
    DEBUG_OUTPUT( "current_block = %p\r\n", ( void* )current_block );
  #endif

  // remove node from used block
  avl_remove_by_data( used_area, address_node->data );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "removed %p from tree\r\n",
      ( void* )&current_block->node_address );

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
  assert(
    avl_insert_by_node( free_address, &current_block->node_address )
    && avl_insert_by_node( free_size, &current_block->node_size )
  )

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Used tree:\r\n" );
    avl_print( used_area );

    DEBUG_OUTPUT( "Free address tree:\r\n" );
    avl_print( free_address );

    DEBUG_OUTPUT( "Free size tree:\r\n" );
    avl_print( free_size );
  #endif

  bool merge_result;
  do {
    merge_result = merge_address_tree( free_address->root );
  } while ( merge_result );

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
