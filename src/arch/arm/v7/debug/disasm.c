
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

#include <string.h>
#include <arch/arm/v7/cpu.h>
#include <core/debug/debug.h>
#include <core/debug/disasm.h>

/**
 * @brief Get next address for stepping
 *
 * @param address
 * @param stack
 * @param context
 * @return uintptr_t*
 *
 * @todo add correct arm / thumb handling if necessary
 * @todo complete logic to cover all branch instructions
 */
uintptr_t* debug_disasm_next_instruction(
  uintptr_t address,
  uintptr_t stack,
  __maybe_unused void* context
) {
  // static array for up to two instruction addresses
  static uintptr_t next_instruction[ DEBUG_DISASM_MAX_INSTRUCTION ];
  // instruction pointer
  uintptr_t instruction = *( ( volatile uintptr_t* )address );
  // clear static return
  memset( ( void* )next_instruction, 0, sizeof( next_instruction ) );
  // index
  uint32_t instruction_index = 0;

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
    // push to array
    next_instruction[ instruction_index++ ] = *sp;
    // return next address
    return next_instruction;

  // branch by "b"
  } else if ( 0xea000000 == ( instruction & 0xff000000 ) ) {
    // FIXME: ADD LOGIC!

  // branch by "bl"
  } else if (
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
    next_instruction[ instruction_index++ ] = ( uintptr_t )(
      ( imm24.data.offset << 2 ) + ( int32_t ) address + 8
    );
    // return next address
    return next_instruction;

  // unconditional branch with bx
  } else if (
    0xe12fff10 == ( instruction & 0xf12fff10 )
  ) {
    // get branch register
    uint32_t reg = instruction & 0x0000000f;
    // get register context
    cpu_register_context_ptr_t cpu = ( cpu_register_context_ptr_t )context;
    // get register to branch to
    next_instruction[ instruction_index++ ] = cpu->raw[ reg ];
    // return next address
    return next_instruction;
  }

  // default next instruction follows current one
  next_instruction[ instruction_index ] = address + 4;
  // return pointer
  return next_instruction;
}
