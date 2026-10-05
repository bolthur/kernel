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

#include "../entry.h"
#include "../lib/assert.h"
#include "../mm/phys.h"
#include "../mm/virt.h"
#include "pool.h"

#include "../lib/inttypes.h"
#include "../debug/debug.h"

/**
 * @brief pool start
 */
static uintptr_t rpc_pool_start;

/**
 * @brief current pool size
 */
static uintptr_t rpc_pool_size;

/**
 * @brief backup list head
 */
static rpc_backup_t* head;

/**
 * @brief backup list tail
 */
static rpc_backup_t* tail;

/**
 * @fn void rpc_pool_setup( void )
 * @brief Function to setup rpc pool
 */
void rpc_pool_setup( void ) {
  // rpc pool starts with one page => ~64 possible rpc
  rpc_pool_start = KERNEL_RPC_POOL_START;
  rpc_pool_size = PAGE_SIZE;
  // map rpc pool
  for ( uintptr_t addr = rpc_pool_start; addr < rpc_pool_start + rpc_pool_size; addr += PAGE_SIZE ) {
    assert( virt_map_address_random(
      virt_current_kernel_context,
      addr,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) );
  }
  // setup head and tail
  head = tail = nullptr;
  // init pool
  rpc_pool_init();
}

/**
 * @fn rpc_backup_t* rpc_pool_pop( void )
 * @brief Pop an rpc from pool
 * @return
 */
rpc_backup_t* rpc_pool_pop( void ) {
  // try to expand
  if ( ! head ) {
    rpc_pool_expand();
  }
  // handle still nothing in
  if ( ! head ) {
    return nullptr;
  }
  rpc_backup_t* backup = head;
  // take last element in list
  if ( head == tail ) {
    head = tail = nullptr;
    return backup;
  }
  // push head to next
  head = head->next;
  // return backup
  return backup;
}

/**
 * @fn void rpc_pool_push( rpc_backup_t* backup )
 * @brief Push an rpc back to pool
 * @param backup
 */
void rpc_pool_push( rpc_backup_t* backup ) {
  if ( ! head ) {
    head = tail = backup;
    return;
  }
  tail->next = backup;
  tail = backup;
}

/**
 * @fn void rpc_pool_init( void )
 * @brief Init rpc pool
 */
void rpc_pool_init( void ) {
  _Static_assert( 64 == sizeof( rpc_backup_t ), "rpc_backup_t must be exactly 64 bytes" );
  for (
    uintptr_t addr = rpc_pool_start;
    addr < rpc_pool_start + rpc_pool_size;
    addr += sizeof( rpc_backup_t )
  ) {
    rpc_pool_push( ( rpc_backup_t* ) addr );
  }
}

/**
 * @fn void rpc_pool_expand( void )
 * @brief expand rpc pool
 */
void rpc_pool_expand( void ) {
  const uintptr_t new_size = rpc_pool_size + PAGE_SIZE;
  // handle limit reached
  if ( rpc_pool_start + new_size > KERNEL_RPC_POOL_END ) {
    return;
  }
  // try map it
  if ( ! virt_map_address_random(
      virt_current_kernel_context,
      rpc_pool_start + rpc_pool_size,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
  ) ) {
    return;
  }
  // setup
  for (
    uintptr_t addr = rpc_pool_start + rpc_pool_size;
    addr < rpc_pool_start + new_size;
    addr += sizeof( rpc_backup_t )
  ) {
    rpc_pool_push( ( rpc_backup_t* ) addr );
  }
  // finally set new size
  rpc_pool_size = new_size;
}
