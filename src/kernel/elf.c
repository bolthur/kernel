
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

#include <kernel/elf.h>
#include <kernel/entry.h>
#include <kernel/debug/debug.h>

/**
 * @brief Check elf header for execution
 *
 * @param header header to check
 * @return true elf header valid
 * @return false elf header invalid
 */
bool elf_check( elf_header_ptr_t header ) {
  // check magic
  if (
    ELF_HEADER_MAGIC_1 != header->magic[ 0 ]
    || ELF_HEADER_MAGIC_2 != header->magic[ 1 ]
    || ELF_HEADER_MAGIC_3 != header->magic[ 2 ]
    || ELF_HEADER_MAGIC_4 != header->magic[ 3 ]
  ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Invalid header magic value!\r\n" );
    #endif
    // return error
    return false;
  }

  // check architecture
  #if defined( ELF32 )
    if ( ELF_HEADER_ARCHITECTURE_32 != header->architecture ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid architecture!\r\n" );
      #endif
      // return error
      return false;
    }
  #elif defined( ELF64 )
    if ( ELF_HEADER_ARCHITECTURE_64 != header->architecture ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid architecture!\r\n" );
      #endif
      // return error
      return false;
    }
  #endif

  // architecture related checks
  if ( ! elf_arch_check( header ) ) {
    return false;
  }

  // ensure program header existance
  if ( 0 == header->program_header_count ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Program header missing!\r\n" );
    #endif
    // return error
    return false;
  }

  // ensure section header existance
  if ( 0 == header->section_header_count ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Section header missing!\r\n" );
    #endif
    // return error
    return false;
  }

  // return success
  return true;
}
