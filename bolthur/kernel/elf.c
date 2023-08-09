/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include "lib/inttypes.h"
#include "lib/string.h"
#include "elf.h"
#include "mm/phys.h"
#include "entry.h"
#if defined( PRINT_ELF )
  #include "debug/debug.h"
#endif

/**
 * @fn bool elf_check(uintptr_t)
 * @brief Check elf header for execution
 *
 * @param elf header address to check
 * @return
 */
bool elf_check( uintptr_t elf ) {
  #if defined( ELF32 )
    Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;
  #elif defined( ELF64 )
    Elf64_Ehdr* header = ( Elf64_Ehdr* )elf;
  #endif

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
  #elif defined( ELF64 )
    if ( ELFCLASS64 != header->e_ident[ EI_CLASS ] ) {
  #endif
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "Invalid architecture!\r\n" )
    #endif
    // return error
    return false;
  }

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
 * @fn bool load_program_header(uintptr_t, task_process_t*)
 * @brief Internal helper to parse and load program header
 *
 * @param elf elf image address
 * @param process process
 * @return
 */
static bool load_program_header( uintptr_t elf, task_process_t* process ) {
  // get header
  #if defined( ELF32 )
    Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;
  #elif defined( ELF64 )
    Elf64_Ehdr* header = ( Elf64_Ehdr* )elf;
  #endif

  // parse program header
  for ( uint32_t index = 0; index < header->e_phnum; ++index ) {
    // get program header
    #if defined( ELF32 )
      Elf32_Phdr* program_header = ( Elf32_Phdr* )(
        elf + header->e_phoff + header->e_phentsize * index
      );
    #elif defined( ELF64 )
      Elf64_Phdr* program_header = ( Elf64_Phdr* )(
        elf + header->e_phoff + header->e_phentsize * index
      );
    #endif
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT(
        "type = %#"PRIx32", vaddr = %#"PRIx32", paddr = %#"PRIx32", "
        "size = %#"PRIx32", offset = %#"PRIx32"!\r\n",
        program_header->p_type,
        program_header->p_vaddr,
        program_header->p_paddr,
        program_header->p_memsz,
        program_header->p_offset
      )
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
    start = ROUND_DOWN_TO_FULL_PAGE( start );
    end = ROUND_UP_TO_FULL_PAGE( end );
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT( "start = %#"PRIxPTR", end = %#"PRIxPTR"\r\n", start, end )
    #endif

    // determine copy offset and copy amount
    uintptr_t memory_offset = program_header->p_vaddr % PAGE_SIZE;
    uintptr_t file_offset = program_header->p_offset;
    uintptr_t copy_size = program_header->p_memsz;
    if ( program_header->p_memsz > program_header->p_filesz ) {
      copy_size = program_header->p_filesz;
    }
    // debug output
    #if defined ( PRINT_ELF )
      DEBUG_OUTPUT(
        "memory_offset = %#"PRIxPTR", file_offset = %#"PRIxPTR
        ", copy_size = %#"PRIxPTR"\r\n",
        memory_offset,
        file_offset,
        copy_size
      )
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
      // cap to copy size
      if ( to_copy > copy_size ) {
        to_copy = copy_size;
      }
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT(
          "copy_size = %#"PRIxPTR", to_copy = %#"PRIxPTR
          ", file_offset = %#"PRIxPTR"\r\n",
          copy_size,
          to_copy,
          file_offset
        )
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
        phys = phys_find_free_page( PAGE_SIZE, PHYS_MEMORY_TYPE_NORMAL );
        // handle error
        if ( 0 == phys ) {
          return false;
        }
        // set clear flag
        clear = true;
      }
      // debug output
      #if defined ( PRINT_ELF )
        DEBUG_OUTPUT( "phys = %#"PRIx64"\r\n", phys )
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
      if ( 0 < to_copy ) {
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
      }
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
 * @fn uintptr_t elf_load(uintptr_t, task_process_t*)
 * @brief Method to load simple elf for process ( used for init only )
 *
 * @param elf address to image
 * @param process process where it shall be loaded into
 * @return
 */
uintptr_t elf_load( uintptr_t elf, task_process_t* process ) {
  // check for elf
  if ( ! elf_check( elf ) ) {
    return 0;
  }
  // get header
  #if defined( ELF32 )
    Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;
  #elif defined( ELF64 )
    Elf64_Ehdr* header = ( Elf64_Ehdr* )elf;
  #else
    #error "Unsupported"
  #endif
  // load program header
  if ( ! load_program_header( elf, process ) ) {
    return 0;
  }
  // return entry
  return ( uintptr_t )header->e_entry;
}

/**
 * @fn size_t elf_image_size(uintptr_t)
 * @brief Helper to get elf image size
 *
 * @param elf
 * @return
 */
size_t elf_image_size( uintptr_t elf ) {
  size_t header_size;
  #if defined( ELF32 )
    header_size = sizeof( Elf32_Ehdr );
  #elif defined( ELF64 )
    header_size = sizeof( Elf64_Ehdr );
  #else
    #error "Unsupported"
  #endif

  // check header
  if ( ! virt_is_mapped_range( elf, header_size ) || ! elf_check( elf ) ) {
    return 0;
  }
  // temporary max and size
  size_t max = 0;
  size_t sz = 0;

  // transform to elf header
  #if defined( ELF32 )
    Elf32_Ehdr* header = ( Elf32_Ehdr* )elf;
  #elif defined( ELF64 )
    Elf64_Ehdr* header = ( Elf64_Ehdr* )elf;
  #else
    #error "Unsupported"
  #endif
  // loop through section header information
  for ( uint32_t idx = 0; idx < header->e_shnum; ++idx ) {
    #if defined( ELF32 )
      Elf32_Shdr* section_header = ( Elf32_Shdr* )(
        elf + header->e_shoff + header->e_shentsize * idx
      );
    #elif defined( ELF64 )
      Elf64_Shdr* section_header = ( Elf64_Shdr* )(
        elf + header->e_shoff + header->e_shentsize * idx
      );
    #else
      #error "Unsupported"
    #endif
    // check if mapped before access
    if ( ! virt_is_mapped_range(
      ( uintptr_t )section_header,
      sizeof( *section_header ) )
    ) {
      return 0;
    }
    if ( section_header->sh_addr > max ) {
      max = section_header->sh_addr;
      sz = section_header->sh_size;
    }
  }
  // return size
  return ( size_t )( header->e_shnum * header->e_shentsize )
    + header->e_shoff + sz;
}
