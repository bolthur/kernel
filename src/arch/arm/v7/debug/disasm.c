
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

#include <core/debug/debug.h>
#include <core/debug/disasm.h>

/**
 * @brief Get next address for stepping
 *
 * @param address
 * @param stack
 * @return uintptr_t
 *
 * @todo add correct arm / thumb handling if necessary
 */
uintptr_t debug_disasm_next_instruction( uintptr_t address, uintptr_t stack ) {
  // instruction pointer
  uintptr_t instruction = *( ( volatile uintptr_t* )address );

  if (
    // check for pop instruction
    0xe8bd0000 == ( instruction & 0xffff0000 )
    // check for pc register is within the instruction
    && ( 1 << 15 ) == ( instruction & ( 1 << 15 ) )
  ) {
    // amount of registers to skip
    uint32_t pop_count = 0;
    for ( uint32_t idx = 0; idx < 16; idx++ ) {
      // increment if set
      if ( ( uintptr_t )( 1 << idx ) & instruction ) {
        pop_count++;
      }
    }
    // get pc from stack
    volatile uint32_t* sp = ( volatile uint32_t* )stack;
    // decrease by one if greater 0
    if ( 0 < pop_count ) {
      pop_count--;
    }
    // increase stack by registers to skip
    sp += pop_count;
    // return next address
    return ( uintptr_t )*sp;
  }

  // branch by "b"
  if ( 0xea000000 == ( instruction & 0xff000000 ) ) {
    // FIXME: ADD LOGIC!
  }

  if (
    // branch by "bl"
    0xeb000000 == ( instruction & 0xff000000 )
  ) {
    // anonymous union for branch offset
    union {
      struct {
        int32_t offset: 24;
        int32_t ignore: 8;
      } data;
      uint32_t raw;
    } imm24;
    // write instruction to raw for special access
    imm24.raw = instruction;
    // return address is determined by offset shifted by 2 bits + address + 8
    // offset of 8 is necessary due to the bl command handled in multiple steps
    uintptr_t return_address = ( uintptr_t )(
      ( imm24.data.offset << 2 ) + ( int32_t ) address + 8
    );
    // return found
    return return_address;
  }

  // return default
  return address + 4;
}
