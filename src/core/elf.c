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

#include <inttypes.h>
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
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT(
        "type = %#x, vaddr = %#x, paddr = %#x, size = %#x, offset = %#x!\r\n",
        program_header->p_type, program_header->p_vaddr, program_header->p_paddr,
        program_header->p_memsz, program_header->p_offset )
    #endif
    // skip all sections except load
    if ( PT_LOAD != program_header->p_type ) {
      continue;
    }

    // determine start and end
    uintptr_t start = program_header->p_vaddr;
    uintptr_t end = start + program_header->p_memsz;
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "start = %#"PRIxPTR", end = %#"PRIxPTR"\r\n", start, end )
    #endif
    // round down start and up end
    ROUND_DOWN_TO_FULL_PAGE( start );
    ROUND_UP_TO_FULL_PAGE( end );
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "start = %#"PRIxPTR", end = %#"PRIxPTR"\r\n", start, end )
    #endif

    // determine copy offset and copy amount
    uintptr_t memory_offset = program_header->p_vaddr % PAGE_SIZE;
    uintptr_t file_offset = program_header->p_offset;
    uintptr_t copy_size = program_header->p_memsz;
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT(
        "memory_offset = %#"PRIxPTR", file_offset = %#"PRIxPTR", copy_size = %#"PRIxPTR"\r\n",
        memory_offset, file_offset, copy_size )
    #endif

    // loop from start to end, map and copy data
    while( start < end ) {
      // determine copy amount
      uintptr_t to_copy = copy_size;
      // handle page size exceeded
      if ( to_copy > PAGE_SIZE ) {
        to_copy = PAGE_SIZE;
      }
      // consider memory offset
      if ( memory_offset > 0 ) {
        to_copy = PAGE_SIZE - memory_offset;
      }
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT(
          "copy_size = %#"PRIxPTR", to_copy = %#"PRIxPTR", file_offset = %#"PRIxPTR"\r\n",
          copy_size, to_copy, file_offset )
      #endif

      // physical page variable and clear flag
      uint64_t phys;
      bool clear = false;
      // handle already mapped ( fetch physical page )
      if ( virt_is_mapped_in_context( process->virtual_context, start ) ) {
        // get physical address of already mapped one
        phys = virt_get_mapped_address_in_context(
          process->virtual_context,
          start
        );
        // handle error
        if ( ( uint64_t )-1 == phys ) {
          return false;
        }
      // handle not mapped ( acquire new physical page )
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
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "phys = %#016llx\r\n", phys )
      #endif

      // map temporary
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
          DEBUG_OUTPUT( "clear page at address %#"PRIxPTR"\r\n", tmp )
        #endif
        // clear page
        memset( ( void* )tmp, 0, PAGE_SIZE );
      }
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT(
          "memcpy( %#"PRIxPTR", %#"PRIxPTR", %#"PRIxPTR" )\r\n",
          tmp + memory_offset,
          ( uintptr_t )header + file_offset,
          to_copy
        )
      #endif
      // copy data
      memcpy(
        ( void* )( tmp + memory_offset ),
        ( void* )( ( uintptr_t )header + file_offset ),
        to_copy
      );
      // unmap temporary again
      virt_unmap_temporary( tmp, PAGE_SIZE );
      // map it within process context if new page
      if ( clear ) {
        // get mapping flag for section
        uint32_t mapping_flag = VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE;
        if ( program_header->p_flags & PF_X ) {
          mapping_flag |= VIRT_PAGE_TYPE_EXECUTABLE;
        }
        // map it
        if ( ! virt_map_address(
            process->virtual_context,
            start,
            phys,
            VIRT_MEMORY_TYPE_NORMAL,
            mapping_flag
          )
        ) {
          // free phys if new page is registered
          phys_free_page( phys );
          // return error
          return false;
        }
      }

      // set offset to 0
      memory_offset = 0;
      // decrease copy size
      copy_size -= to_copy;
      // increase file offset
      file_offset += to_copy;
      // increase start
      start += PAGE_SIZE;
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
