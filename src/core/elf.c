
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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
#include <core/elf/common.h>
#include <core/elf/elf32.h>
#include <core/mm/phys.h>
#include <core/entry.h>
#include <core/debug/debug.h>

/**
 * @brief Check elf header for execution
 *
 * @param elf header address to check
 * @return true elf header valid
 * @return false elf header invalid
 */
bool elf_check( uintptr_t elf ) {
  Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;

  // handle invalid
  if ( NULL == header ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Invalid parameter!\r\n" );
    #endif
    // return error
    return false;
  }

  // check magic
  if (
    ELFMAG0 != header->e_ident[ EI_MAG0 ]
    || ELFMAG1 != header->e_ident[ EI_MAG1 ]
    || ELFMAG2 != header->e_ident[ EI_MAG2 ]
    || ELFMAG3 != header->e_ident[ EI_MAG3 ]
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
    if ( ELFCLASS32 != header->e_ident[ EI_CLASS ] ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid architecture!\r\n" );
      #endif
      // return error
      return false;
    }
  #elif defined( ELF64 )
    if ( ELFCLASS64 != header->e_ident[ EI_CLASS ] ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid architecture!\r\n" );
      #endif
      // return error
      return false;
    }
  #endif

  // architecture related checks
  if ( ! elf_arch_check( elf ) ) {
    return false;
  }

  // ensure program header existence
  if ( 0 == header->e_phentsize ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Program header missing!\r\n" );
    #endif
    // return error
    return false;
  }

  // ensure section header existence
  if ( 0 == header->e_shentsize ) {
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

/**
 * @brief Internal helper to parse and load program header
 *
 * @param elf adress to elf header
 * @param process process structure
 */
static void load_program_header( uintptr_t elf, task_process_ptr_t process ) {
  // get header
  Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;

  // parse program header
  for ( uint32_t index = 0; index < header->e_phnum; ++index ) {
    // get program header
    Elf32_Phdr* program_header = ( Elf32_Phdr* )(
      elf + header->e_phoff + header->e_phentsize * index
    );

    // skip non loadable sections
    if ( PT_LOAD != program_header->p_type ) {
      continue;
    }

    // determine needed space
    uintptr_t needed_size = program_header->p_memsz;
    ROUND_UP_TO_FULL_PAGE( needed_size )

    // loop until needed size is zero
    uintptr_t offset = 0;
    while ( needed_size != 0 ) {
      // get physical page
      uint64_t phys = phys_find_free_page( PAGE_SIZE );

      // map it temporary
      uintptr_t tmp = virt_map_temporary( phys, PAGE_SIZE );

      // partial data copy
      if ( offset < program_header->p_filesz ) {
        // determine amount to copy
        size_t to_copy = program_header->p_filesz - offset;
        // copy up to one page size
        if ( to_copy > PAGE_SIZE ) {
          to_copy = PAGE_SIZE;
        }
        // copy over data
        memcpy(
          ( void* )tmp,
          ( void* )( ( uintptr_t )header + program_header->p_offset + offset ),
          program_header->p_filesz - offset
        );
      }

      // unmap temporary
      virt_unmap_temporary( tmp, PAGE_SIZE );

      // map it wotjom process context
      virt_map_address(
        process->virtual_context,
        program_header->p_vaddr + offset,
        phys,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_EXECUTABLE
      );

      // subtract one page
      needed_size -= PAGE_SIZE;
      offset += PAGE_SIZE;
    }
  }
}

/**
 * @brief Method to load elf for process
 *
 * @param elf address to elf header
 * @param process process structure
 * @return uintptr_t program entry or 0 on error
 */
uintptr_t elf_load( uintptr_t elf, task_process_ptr_t process ) {
  // check for elf
  if ( ! elf_check( elf ) ) {
    return 0;
  }
  // get header
  Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;
  // load program header
  load_program_header( elf, process );
  // return entry
  return ( uintptr_t )header->e_entry;
}
