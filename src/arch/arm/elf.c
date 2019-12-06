
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
bool elf_arch_check( elf_header_ptr_t header ) {
  // check machine to match kernel
  #if defined( ELF32 )
    if ( ELF_HEADER_MACHINE_ARM != header->machine ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid machine type, expected %x and received %x!\r\n",
          ELF_HEADER_MACHINE_ARM, header->machine );
      #endif
      // return error
      return false;
    }
  #elif defined( ELF64 )
    if ( ELF_HEADER_MACHINE_AARCH64 != header->machine ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid machine type!\r\n" );
      #endif
      // return error
      return false;
    }
  #endif

  // return success
  return true;
}
