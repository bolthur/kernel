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

#include <inttypes.h>
#include "kasan.h"
#include "../assert.h"
#include "../string.h"
#include "../../../application/usr/lib/ld-bolthur/tmp/_dl-int.h"
#include "../../debug/debug.h"
#include "../../mm/virt.h"

uintptr_t kasan_shadow_memory_start = 0;
uintptr_t kasan_shadow_memory_end = 0;

/**
 * @fn uintptr_t kasan_get_poisoned_shadow_address(uintptr_t, size_t)
 * @brief Wrapper to get poisoned shadow address
 * @param addr
 * @param size
 * @return
 */
uintptr_t kasan_get_poisoned_shadow_address(
  const uintptr_t addr,
  const size_t size
) {
  const uintptr_t addr_shadow_start = KASAN_MEM_TO_SHADOW( addr );
  const uintptr_t addr_shadow_end = KASAN_MEM_TO_SHADOW( addr + size - 1 ) + 1;
  uintptr_t non_zero_shadow_addr = 0;

  for ( uintptr_t i = 0; i < addr_shadow_end - addr_shadow_start; i++ ) {
    if ( *( uint8_t* )( addr_shadow_start + i ) ) {
      non_zero_shadow_addr = addr_shadow_start + i;
      break;
    }
  }

  if ( non_zero_shadow_addr ) {
    const uintptr_t last_byte = addr + size - 1;
    const int8_t *last_shadow_byte = ( int8_t* )KASAN_MEM_TO_SHADOW( last_byte );

    // Non-zero bytes in shadow memory may indicate either:
    //  1) invalid memory access (0xff, 0xfa, ...)
    //  2) access to a 8-byte region which isn't entirely accessible, i.e. only
    //     n bytes can be read/written in the 8-byte region, where n < 8
    //     (in this case shadow byte encodes how much bytes in an 8-byte region
    //     are accessible).
    // Thus, if there is a non-zero shadow byte we need to check if it
    // corresponds to the last byte in the checked region:
    //   not last - OOB memory access
    //   last - check if we don't access beyond what's encoded in the shadow
    //          byte.
    if ( non_zero_shadow_addr != ( uintptr_t )last_shadow_byte
      || ( int8_t )( last_byte & KASAN_SHADOW_MASK ) >= *last_shadow_byte
    ) {
      return non_zero_shadow_addr;
    }
  }

  return 0;
}

/**
 * @fn void kasan_poison_shadow(uintptr_t, size_t, uint8_t, bool)
 * @brief Poison shadow with map if set
 * @param addr
 * @param size
 * @param val
 * @param map
 */
void kasan_poison_shadow(
  const uintptr_t addr,
  const size_t size,
  const uint8_t val,
  const bool map
) {
  const uintptr_t shadow_start = KASAN_MEM_TO_SHADOW( addr );
  const uintptr_t shadow_end = KASAN_MEM_TO_SHADOW( addr + size - 1 ) + 1;
  const size_t shadow_length = shadow_end - shadow_start;
  // perform map if set
  if ( map ) {
    // set start if not set
    if ( kasan_shadow_memory_start == 0 ) {
      kasan_shadow_memory_start = ROUND_DOWN_TO_FULL_PAGE( shadow_start );
    }
    // map it
    for (
      uintptr_t map_addr = ROUND_DOWN_TO_FULL_PAGE( shadow_start );
      map_addr < shadow_end;
      map_addr += PAGE_SIZE
    ) {
      // handle already mapped
      if ( virt_is_mapped( map_addr ) ) {
        continue;
      }
      // map it
      assert( virt_map_address_random(
        virt_current_kernel_context,
        map_addr,
        VIRT_MEMORY_TYPE_NORMAL_NC,
        VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
      ) );
      // adjust end
      kasan_shadow_memory_end = map_addr + PAGE_SIZE;
    }
  }
  memset( ( void* )shadow_start, val, shadow_length );
}

/**
 * @fn void kasan_unpoison_shadow(uintptr_t, size_t)
 * @brief Wrapper to unpoison shadow address
 * @param address
 * @param size
 */
void kasan_unpoison_shadow( const uintptr_t address, const size_t size ) {
  kasan_poison_shadow(
    address,
    size & ~KASAN_SHADOW_MASK,
    ASAN_SHADOW_UNPOISONED_MAGIC,
    false
  );
  if (size & KASAN_SHADOW_MASK) {
    auto const shadow = ( uint8_t* )KASAN_MEM_TO_SHADOW( address + size );
    *shadow = size & KASAN_SHADOW_MASK;
  }
}

/**
 * @fn int kasan_check_memory(uintptr_t, size_t, bool, uintptr_t)
 * @brief Kasan check memory helper
 * @param addr
 * @param size
 * @param write
 * @param pc
 * @return
 */
int kasan_check_memory(
  const uintptr_t addr,
  const size_t size,
  const bool write,
  const uintptr_t pc
) {
  // handle no size
  if ( ! size ) {
    return 1;
  }
  // handle not in memory
  if ( addr < HEAP_START || addr > HEAP_START + HEAP_MAX_SIZE ) {
    return 1;
  }
  // get poisoned shadow address
  const uintptr_t buggy_shadow_address = kasan_get_poisoned_shadow_address(
    addr, size );
  if ( ! buggy_shadow_address ) {
    return 1;
  }
  // report bug
  kasan_bug_report( addr, size, buggy_shadow_address, write, pc );
  return 0;
}

/**
 * @fn void kasan_init(void)
 * @brief Kasan init method
 */
void kasan_init( void ) {
  // poison heap
  kasan_poison_shadow(
    HEAP_START,
    HEAP_MIN_SIZE,
    ASAN_SHADOW_RESERVED_MAGIC,
    true
  );
}

void __asan_handle_no_return( void ) {}

void __asan_store1_noabort( uintptr_t address ) {
  kasan_check_memory( address, 1, true, KASAN_CALLER_PC );
}

void __asan_store2_noabort( uintptr_t address ) {
  kasan_check_memory( address, 2, true, KASAN_CALLER_PC );
}

void __asan_store4_noabort( uintptr_t address ) {
  kasan_check_memory( address, 4, true, KASAN_CALLER_PC );
}

void __asan_store8_noabort( uintptr_t address ) {
  kasan_check_memory( address, 8, true, KASAN_CALLER_PC );
}

void __asan_store16_noabort( uintptr_t address ) {
  kasan_check_memory( address, 16, true, KASAN_CALLER_PC );
}

void __asan_storeN_noabort( uintptr_t address, size_t size ) {
  kasan_check_memory( address, size, true, KASAN_CALLER_PC );
}

void __asan_load1_noabort( uintptr_t address ) {
  kasan_check_memory( address, 1, false, KASAN_CALLER_PC );
}

void __asan_load2_noabort( uintptr_t address ) {
  kasan_check_memory( address, 2, false, KASAN_CALLER_PC );
}

void __asan_load4_noabort( uintptr_t address ) {
  kasan_check_memory( address, 4, false, KASAN_CALLER_PC );
}

void __asan_load8_noabort( uintptr_t address ) {
  kasan_check_memory( address, 8, false, KASAN_CALLER_PC );
}

void __asan_load16_noabort( uintptr_t address ) {
  kasan_check_memory( address, 16, false, KASAN_CALLER_PC );
}

void __asan_loadN_noabort( uintptr_t address, size_t size ) {
  kasan_check_memory( address, size, false, KASAN_CALLER_PC );
}
