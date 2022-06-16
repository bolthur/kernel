/**
 * Copyright (C) 2018 - 2022 bolthur project.
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
#include "../../elf.h"
#include "../../entry.h"
#if defined ( PRINT_ELF )
  #include "../../debug/debug.h"
#endif

/**
 * @brief Check elf header for execution
 *
 * @param elf header address to check
 * @return true elf header valid
 * @return false elf header invalid
 */
bool elf_arch_check( uintptr_t elf ) {
  Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;

  // check machine to match kernel
  #if defined( ELF32 )
    if ( EM_ARM != header->e_machine ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT(
          "Invalid machine type, expected %x and received %x!\r\n",
          EM_ARM,
          header->e_machine
        )
      #endif
      // return error
      return false;
    }
  #elif defined( ELF64 )
    if ( EM_AARCH64 != header->e_machine ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT(
          "Invalid machine type, expected %x and received %x!\r\n",
          EM_AARCH64,
          header->e_machine
        )
      #endif
      // return error
      return false;
    }
  #endif

  // return success
  return true;
}
