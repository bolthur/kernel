
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

#include <stdbool.h>
#include <string.h>
#include <core/elf/common.h>
#include <core/elf/elf32.h>
#include <core/mm/phys.h>
#include <core/entry.h>
#if defined( PRINT_ELF )
  #include <core/debug/debug.h>
#endif

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
  if ( ! header ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Invalid parameter!\r\n" )
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
      DEBUG_OUTPUT( "Invalid header magic value!\r\n" )
    #endif
    // return error
    return false;
  }

  // check architecture
  #if defined( ELF32 )
    if ( ELFCLASS32 != header->e_ident[ EI_CLASS ] ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid architecture!\r\n" )
      #endif
      // return error
      return false;
    }
  #elif defined( ELF64 )
    if ( ELFCLASS64 != header->e_ident[ EI_CLASS ] ) {
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "Invalid architecture!\r\n" )
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
      DEBUG_OUTPUT( "Program header missing!\r\n" )
    #endif
    // return error
    return false;
  }

  // ensure section header existence
  if ( 0 == header->e_shentsize ) {
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Section header missing!\r\n" )
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
 * @param elf address to elf header
 * @param process process structure
 * @return true
 * @return false
 */
static bool load_program_header( uintptr_t elf, task_process_ptr_t process ) {
  // get header
  Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;

  // parse program header
  for ( uint32_t index = 0; index < header->e_phnum; ++index ) {
    // get program header
    Elf32_Phdr* program_header = ( Elf32_Phdr* )(
      elf + header->e_phoff + header->e_phentsize * index
    );

    // skip all sections except load
    if ( PT_LOAD != program_header->p_type ) {
      continue;
    }

    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT(
        "type = %#x, vaddr = %#x, paddr = %#x, size = %#x, offset = %#x!\r\n",
        program_header->p_type,
        program_header->p_vaddr,
        program_header->p_paddr,
        program_header->p_memsz,
        program_header->p_offset )
    #endif

    // determine needed space
    uintptr_t needed_size = program_header->p_memsz;
    // debug output
    #if defined( PRINT_ELF )
      DEBUG_OUTPUT( "needed_size = %#x\r\n", needed_size )
    #endif
    // round to full page
    ROUND_UP_TO_FULL_PAGE( needed_size )
    // debug output
    #if defined( PRINT_ELF )
      DEBUG_OUTPUT( "needed_size = %#x\r\n", needed_size )
    #endif

    // get start address
    uintptr_t start = program_header->p_vaddr;
    uintptr_t start_down = start;
    uintptr_t start_up = start;
    // debug output
    #if defined( PRINT_ELF )
      DEBUG_OUTPUT( "needed_size = %#x\r\n", needed_size )
    #endif
    // check for not aligned address
    if ( start_down % PAGE_SIZE ) {
      // round down start to full page
      ROUND_DOWN_TO_FULL_PAGE( start_down )
      // check if mapped
      if ( virt_is_mapped_in_context(
        process->virtual_context,
        start_down
      ) ) {
        // subtract already mapped size from size
        ROUND_UP_TO_FULL_PAGE( start_up )
        needed_size -= ( start_up - start );
      } else {
        // enlarge needed size
        needed_size += ( start - start_down );
      }
    }
    // round again up to full page due to possible modifications
    ROUND_UP_TO_FULL_PAGE( needed_size )
    // debug output
    #if defined( PRINT_ELF )
      DEBUG_OUTPUT( "needed_size = %#x\r\n", needed_size )
    #endif

    // loop until needed size is zero
    uintptr_t offset = 0;
    uintptr_t page_offset = program_header->p_vaddr % PAGE_SIZE;

    while ( needed_size != 0 ) {
      // physical page
      uint64_t phys;
      bool clear = false;

      // determine whether new page has to be mapped or not
      if ( virt_is_mapped_in_context(
        process->virtual_context,
        program_header->p_vaddr + offset
      ) ) {
        phys = virt_get_mapped_address_in_context(
          process->virtual_context,
          program_header->p_vaddr + offset
        );
        // handle error
        if ( ( uint64_t )-1 == phys ) {
          return false;
        }
      } else {
        // get new physical page
        phys = phys_find_free_page( PAGE_SIZE );
        // handle error
        if ( 0 == phys ) {
          return false;
        }
        // set clear flag
        clear = true;
      }

      // map it temporary
      uintptr_t tmp = virt_map_temporary( phys, PAGE_SIZE );
      if ( 0 == tmp ) {
        // free phys if new page is registered
        if ( clear ) {
          phys_free_page( phys );
        }
        // return error
        return false;
      }
      // handle clear
      if ( clear ) {
        // debug output
        #if defined( PRINT_ELF )
          DEBUG_OUTPUT( "clear page at address %#x\r\n", tmp )
        #endif
        memset( ( void* )tmp, 0, PAGE_SIZE );
      }
      // get size to copy
      size_t to_copy = program_header->p_filesz;
      size_t tmp_offset = 0;
      // adjust to copy by file offset and page offset
      if ( offset > 0 ) {
        to_copy -= offset + page_offset;
      }
      // copy up to one page size
      if ( to_copy > PAGE_SIZE ) {
        to_copy = PAGE_SIZE;
      }
      // handle page offset at the beginning
      if ( 0 == offset && 0 < page_offset ) {
        // reduce to copy value
        to_copy = PAGE_SIZE - page_offset;
        tmp_offset = page_offset;
      }
      // debug output
      #if defined( PRINT_ELF )
        DEBUG_OUTPUT(
          "to_copy = %#x, tmp_offset = %#x, offset = %#x\r\n",
          to_copy, tmp_offset, offset)
      #endif
      // copy over data
      memcpy(
        ( void* )( ( uintptr_t )tmp + tmp_offset ),
        ( void* )( ( uintptr_t )header + program_header->p_offset + offset ),
        to_copy
      );
      // unmap temporary
      virt_unmap_temporary( tmp, PAGE_SIZE );

      // get mapping flag from section
      uint32_t mapping_flag =
        program_header->p_flags & PF_X
          ? VIRT_PAGE_TYPE_EXECUTABLE
          : VIRT_PAGE_TYPE_NON_EXECUTABLE;

      // map it within process context
      if ( ! virt_map_address(
          process->virtual_context,
          program_header->p_vaddr + offset,
          phys,
          VIRT_MEMORY_TYPE_NORMAL,
          mapping_flag
        )
      ) {
        // free phys if new page is registered
        if ( clear ) {
          phys_free_page( phys );
        }
        // return error
        return false;
      }

      // subtract one page
      needed_size -= PAGE_SIZE;
      offset += to_copy;
    }
  }
  return true;
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
  if ( ! load_program_header( elf, process ) ) {
    return 0;
  }
  // return entry
  return ( uintptr_t )header->e_entry;
}
