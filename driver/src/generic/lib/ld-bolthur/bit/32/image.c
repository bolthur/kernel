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

#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "../../image.h"
#include "../../debug.h"
#include "image.h"

/**
 * @fn bool image_validate(void*, int)
 * @brief Image validation for 32 bit architecture
 *
 * @param img
 * @param type
 * @return
 */
bool image_validate( void* img, int type ) {
  Elf32_Ehdr* header = ( Elf32_Ehdr* )img;
  // handle invalid
  if ( ! header ) {
    return false;
  }
  // check magic
  if (
    ELFMAG0 != header->e_ident[ EI_MAG0 ]
    || ELFMAG1 != header->e_ident[ EI_MAG1 ]
    || ELFMAG2 != header->e_ident[ EI_MAG2 ]
    || ELFMAG3 != header->e_ident[ EI_MAG3 ]
  ) {
    // return error
    return false;
  }
  // handle wrong architecture
  if ( ELFCLASS32 != header->e_ident[ EI_CLASS ] ) {
    return false;
  }
  // architecture related checks
  if ( ! image_validate_machine( img ) ) {
    return false;
  }
  // ensure program header existence
  if ( 0 == header->e_phentsize ) {
    return false;
  }
  // ensure section header existence
  if ( 0 == header->e_shentsize ) {
    return false;
  }
  if ( type != header->e_type ) {
    return false;
  }
  // return success
  return true;
}

/**
 * @fn bool image_list_contain(list_manager_ptr_t, char*)
 * @brief Method to check whether name has already been loaded
 *
 * @param list
 * @param name
 * @return
 */
bool image_list_contain( list_manager_ptr_t list, char* name ) {
  list_item_ptr_t current = list->first;
  while ( current ) {
    // get image pointer
    elf_image_ptr_t image = current->data;
    // compare names if set
    if ( image->name && 0 == strcmp( image->name, name ) ) {
      #if defined( OUTPUT_ENABLE )
        DEBUG_OUTPUT( "name %s already loaded!\r\n", name );
      #endif
      return true;
    }
    // get to next
    current = current->next;
  }
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "name %s not yet loaded!\r\n", name );
  #endif
  return false;
}

/**
 * @fn bool image_list_data_create(list_manager_ptr_t, void*, size_t, int, char*)
 * @brief Helper to create image structure for list data
 *
 * @param list
 * @param img
 * @param size
 * @param type
 * @return
 */
bool image_list_data_create(
  list_manager_ptr_t list,
  void* img,
  size_t size,
  int type,
  char* name
) {
  elf_image_ptr_t image;
  uintptr_t min_address = UINTPTR_MAX;
  uintptr_t max_address = 0;
  // handle no list
  if ( ! list ) {
    return false;
  }
  // validate
  if ( ! image_validate( img, type ) ) {
    return false;
  }
  // allocate image entry
  image = malloc( sizeof( elf_image_t ) );
  if ( ! image ) {
    return false;
  }
  // erase memory
  memset( image, 0, sizeof( elf_image_t ) );
  // populate image
  image->header = ( Elf32_Ehdr* )img;
  image->size = size;
  if ( name ) {
    // allocate name
    image->name = malloc( sizeof( char ) * strlen( name ) + 1 );
    if ( ! image->name ) {
      free( image );
      return false;
    }
    // copy over name
    strcpy( image->name, name );
  }
  // loop program header and determine min and max address
  for ( Elf32_Half idx = 0; idx < image->header->e_phnum; idx++ ) {
    // get current program header
    Elf32_Phdr* program_header = ( Elf32_Phdr* )(
      ( uintptr_t )img + image->header->e_phoff
        + image->header->e_phentsize * idx
    );
    // skip non loadable header
    if ( PT_LOAD != program_header->p_type ) {
      continue;
    }
    // set min address if applicable
    if ( program_header->p_vaddr < min_address ) {
      min_address = program_header->p_vaddr;
    }
    // set max address if applicable
    if ( program_header->p_vaddr + program_header->p_memsz > max_address ) {
      max_address = program_header->p_vaddr + program_header->p_memsz;
    }
  }
  // populate minimum and maximum address
  image->min_address = min_address;
  image->max_address = max_address;
  // push to list
  if ( ! list_push_back( list, image ) ) {
    free( image->name );
    free( image );
    return false;
  }
  // return created image
  return true;
}

/**
 * @fn void image_destroy_data_entry(elf_image_ptr_t)
 * @brief Helper to destroy image data entry
 *
 * @param image
 */
void image_destroy_data_entry( elf_image_ptr_t image ) {
  if ( ! image ) {
    return;
  }
  if ( image->memory ) {
    free( image->memory );
  }
  if ( image->name ) {
    free( image->name );
  }
  free( image );
}

/**
 * @fn bool image_load(void*, size_t)
 * @brief Load image with dependencies
 *
 * @param img
 * @param size
 * @return
 */
bool image_load( void* img, size_t size ) {
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Create new image list for loading everything!\r\n" )
  #endif
  // create new list for image stuff
  list_manager_ptr_t image_list = list_construct(
    NULL, image_list_cleanup_helper );
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "Push back main image to load!\r\n" )
  #endif
  // Add current image as first one to list
  if ( ! image_list_data_create( image_list, img, size, ET_EXEC, NULL ) ) {
    return false;
  }

  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT(
      "Looping through images, handling normal load and dynamic stuff!\r\n" )
  #endif
  // loop through images extract flat memory and load dynamic stuff if necessary
  list_item_ptr_t current = image_list->first;
  while( current ) {
    #if defined( OUTPUT_ENABLE )
      DEBUG_OUTPUT( "Handle flat load areas!\r\n" )
    #endif
    // handle flat load areas
    if ( ! image_handle_flat( current->data ) ) {
      list_destruct( image_list );
      return false;
    }
    #if defined( OUTPUT_ENABLE )
      DEBUG_OUTPUT( "Handle possible dynamic dependencies!\r\n" )
    #endif
    // handle dynamic dependecies
    if ( ! image_handle_dynamic( current->data, image_list ) ) {
      list_destruct( image_list );
      return false;
    }
    // get to next
    current = current->next;
  }

  // FIXME: RELOCATE STUFF
  // FIXME: ASSIGN ALL IMAGES

  return true;
}

/**
 * @fn void elf_section*(Elf32_Ehdr*, size_t_Word)
 * @brief Helper to get section address
 *
 * @param image
 * @param type
 */
void* image_get_section_by_type( void* image, size_t type ) {
  Elf32_Half idx = 0;
  Elf32_Phdr* program_header = NULL;
  Elf32_Ehdr* header = ( Elf32_Ehdr* )image;
  // loop program header and return section if existing
  while ( idx < header->e_phnum ) {
    // get current program header
    program_header = ( Elf32_Phdr* )(
      ( uintptr_t )header
        + header->e_phoff
        + header->e_phentsize * idx
    );
    // debug output
    #if defined( OUTPUT_ENABLE )
      DEBUG_OUTPUT( "program_header->p_type = %#lx, wanted type = %#x\r\n",
        program_header->p_type, type )
    #endif
    // stop on first dyn program header
    if ( ( Elf32_Word )type == program_header->p_type ) {
      // return found address
      return ( void* )( ( char* )image + program_header->p_offset );
    }
    // increment index
    idx++;
  }
  // nothing found, return null
  return NULL;
}

/**
 * @fn bool image_handle_dynamic(elf_image_ptr_t, list_manager_ptr_t)
 * @brief
 *
 * @param image
 * @param list
 * @return
 */
bool image_handle_dynamic( elf_image_ptr_t image, list_manager_ptr_t list ) {
  // dynamic section pointer
  size_t memory_size = image->max_address - image->min_address;
  Elf32_Dyn* dyn = ( Elf32_Dyn* )image_get_section_by_type(
    image->header, PT_DYNAMIC );
  if ( ! dyn ) {
    return true;
  }

  // necessary data for loading everything
  Elf32_Addr strtab = 0;
  Elf32_Word strsz = 0;
  Elf32_Dyn* current = dyn;
  // loop through dyn section entries and populate some necessary data
  while( current->d_tag != DT_NULL ) {
    switch( current->d_tag ) {
      case DT_STRTAB:
        strtab = ( uintptr_t )current->d_un.d_ptr - image->min_address;
        break;
      case DT_STRSZ:
        strsz = current->d_un.d_val;
        break;
    }
    // get to next one
    current++;
  }

  // validate string table
  if ( strsz > UINT32_MAX - strtab || strtab + strsz > memory_size ) {
    return false;
  }
  // set local pointer for strtab
  char* str = ( char* )&image->memory[ strtab ];
  // ensure NULL termination
  if ( '\0' != str[ strsz - 1 ] ) {
    return false;
  }

  // load needed images
  for ( current = dyn; current->d_tag != DT_NULL; current++ ) {
    // skip everything except DT_NEEDED
    if ( DT_NEEDED != current->d_tag ) {
      continue;
    }
    // string table offset for lib name
    Elf32_Word libname_offset = current->d_un.d_val;
    char* name = str + libname_offset;

    // skip if already loaded
    if ( image_list_contain( list, name ) ) {
      continue;
    }

    // build full path
    size_t sz = strlen( "/lib/" ) + strlen( name ) + 1;
    char* file = malloc( sz );
    if ( ! file ) {
      return false;
    }
    strcpy( file, "/lib/" );
    strcat( file, name );

    // print library
    #if defined( OUTPUT_ENABLE )
      DEBUG_OUTPUT( "NEEDED LIBRARY: %s ( %s )\r\n", file, name )
    #endif

    size_t buffer_size;
    // load executable image
    uint8_t* buffer_image = image_buffer_file( file, &buffer_size );
    // handle image
    if ( ! buffer_image ) {
      fprintf( stderr, "Unable to load image \"%s\"!\r\n", file );
      return false;
    }
    // debug output
    #if defined( OUTPUT_ENABLE )
      DEBUG_OUTPUT( "Image \"%s\" is valid for execution!\r\n", file )
    #endif
    // Add image to list
    if ( ! image_list_data_create( list, buffer_image, buffer_size, ET_DYN, name ) ) {
      return false;
    }
  }

  return true;
}

/**
 * @fn bool image_handle_flat(elf_image_ptr_t)
 * @brief Handle load of flat memory
 *
 * @param image
 * @return
 */
bool image_handle_flat( elf_image_ptr_t image ) {
  // determine size of memory
  size_t memory_size = image->max_address - image->min_address;
  // debug output
  #if defined( OUTPUT_ENABLE )
    DEBUG_OUTPUT( "memory_size = %#x!\r\n", memory_size )
  #endif
  // allocate necessary memory
  uint8_t* memory = malloc( memory_size );
  if ( ! memory ) {
    return false;
  }
  // erase
  memset( memory, 0, memory_size );
  // loop program header and extract loadable content
  for ( Elf32_Half idx = 0; idx < image->header->e_phnum; idx++ ) {
    // get current program header
    Elf32_Phdr* program_header = ( Elf32_Phdr* )(
      ( uintptr_t )image->header
        + image->header->e_phoff
        + image->header->e_phentsize * idx
    );
    // skip non loadable header
    if ( PT_LOAD != program_header->p_type ) {
      continue;
    }
    // copy data into temporary memory
    memcpy(
      &memory[ program_header->p_vaddr - image->min_address ],
      ( uint8_t* )( ( uintptr_t )image->header + program_header->p_offset ),
      program_header->p_filesz
    );
  }
  // push back allocated pointer
  image->memory = memory;
  // return success
  return true;
}
