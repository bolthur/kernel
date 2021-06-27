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

#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <inttypes.h>
#include "_dl-int.h"
#include "debug.h"

#include <sys/mman.h>

// FIXME: use __attribute__((visibility ("hidden"))) for all internal functions

#define E_DL_CANNOT_OPEN 1 /* Cannot open */
#define E_DL_NOT_FOUND 2 /* File not found */
#define E_DL_NO_MEMORY 3 /* Not enough space */
#define E_DL_READ_HDR 4 /* Failed reading header */
#define E_DL_HEADER_VALIDATE_MAGIC 5 /* Wrong elf magic */
#define E_DL_HEADER_VALIDATE_BYTE 6 /* Wrong elf byte class */
#define E_DL_HEADER_VALIDATE_MACHINE 7 /* Wrong elf machine type */
#define E_DL_HEADER_VALIDATE_SECTION 8 /* Wrong elf machine type */
#define E_DL_HEADER_VALIDATE_PROGRAM 9 /* Wrong elf machine type */
#define E_DL_COMPRESSED_NOT_SUPPORTED 10 /* Compressed libraries not supported */
#define E_DL_IO_ERROR 11 /* I/O ERROR */
#define E_DL_NOT_SUPPORTED 12 /* Not supported behaviour */
#define E_DL_UNKNOWN 13 /* Unknown error */

// internal structure containing single message
struct _dl_message_entry {
  char* message;
};
typedef struct _dl_message_entry _dl_message_entry_t;

static _dl_message_entry_t _dl_error_message[] = {
  { "Could not open library: " },
  { "Could not find library: " },
  { "Error while allocating memory" },
  { "Error while reading header" },
  { "Wrong elf magic / not an elf object" },
  { "Wrong elf byte class" },
  { "Wrong elf machine" },
  { "No program header found" },
  { "No section header found" },
  { "Compressed libraries not supported" },
  { "Not supported behaviour" },
  { "Unknown error" },
};

// globals
// FIXME: PUSH VARIABLES TO REENT WHEN IMPORTING TO NEWLIB
uint32_t _dl_error;
const char* _dl_error_location;
const char* _dl_error_data;
dl_image_handle_ptr_t root_object_handle = NULL;

static char dl_open_buffer[ PATH_MAX ];

/**
 * @fn char dlerror*(void)
 * @brief Method to get the possible set error code of dynamic linking
 *
 * @return
 */
char* dlerror( void ) {
  // static buffer
  static char buffer[ 1024 ];
  // set total length to length - 1 to leave space for 0 termination
  size_t total_length = sizeof( buffer ) - 1;
  // handle no error
  if ( 0 == _dl_error ) {
    strncpy( buffer, "no error", total_length );
    // reset globals
    _dl_error_location = 0;
    _dl_error_data = "";
    _dl_error = 0;
    // return buffer
    return buffer;
  }
  // clear buffer
  memset( buffer, 0, sizeof( buffer ) );
  // determine entry count
  size_t error_count = sizeof( _dl_error_message ) / sizeof( _dl_message_entry_t );
  // handle invalid error code
  if ( _dl_error >= error_count ) {
    // copy over last error message
    strncpy(
      buffer,
      _dl_error_message[ error_count - 1 ].message,
      total_length
    );
    // reset globals
    _dl_error_location = 0;
    _dl_error_data = "";
    _dl_error = 0;
    // return buffer
    return buffer;
  }
  // valid error code fill buffer
  char *buffer_pos = buffer;
  size_t length;
  // check if location is set
  if ( _dl_error_location ) {
    // determine length of error location
    length = strlen( _dl_error_location );
    // push location string to buffer
    strncpy( buffer_pos, _dl_error_location, total_length );
    // decrement total length and increment buffer position
    total_length -= length;
    buffer_pos += length;
    // add colon separator
    strncpy( buffer_pos, ": ", total_length );
    // decrement total length and increment buffer position
    total_length -= 2;
    buffer_pos += 2;
  }
  // get length of message
  length = strlen( _dl_error_message[ _dl_error - 1 ].message );
  // push message string to buffer
  strncpy( buffer_pos, _dl_error_message[ _dl_error - 1 ].message, total_length );
  // decrement total length and increment buffer position
  total_length -= length;
  buffer_pos += length;
  // push error data if set
  if ( _dl_error_data ) {
    strncpy( buffer_pos, _dl_error_data, total_length );
  }
  // reset globals
  _dl_error_location = 0;
  _dl_error_data = "";
  _dl_error = 0;
  // return buffer
  return buffer;
}

/**
 * @fn uint32_t dl_elf_symbol_name_hash(const char*)
 * @brief Helper to build elf symbol name hash
 *
 * @param name
 * @return
 */
uint32_t dl_elf_symbol_name_hash( const char* name ) {
  uint32_t h = 0;
  while( *name ) {
    h = ( h << 4 ) + *name++;
    uint32_t g = h & 0xf0000000;
    if ( g ) {
      h ^= g >> 24;
    }
    h &= ~g;
  }
  return h;
}

/**
 * @fn uint32_t dl_elf_symbol_name_hash(const char*)
 * @brief Helper to build elf symbol name hash
 *
 * @param name
 * @return
 *
 * @todo implement logic
 */
uint32_t dl_gnu_symbol_name_hash( __unused const char* name ) {
  return 0;
}

/**
 * @fn void dl_lookup_symbol*(dl_image_handle_ptr_t, const char*)
 * @brief Helper to lookup a symbol
 *
 * @param handle
 * @param name
 */
void* dl_lookup_symbol( dl_image_handle_ptr_t handle, const char* name ) {
  void* found_symbol = NULL;
  dl_image_handle_ptr_t current = handle;
  if ( ! handle ) {
    current = root_object_handle;
  }
  // loop through handles
  while ( current && ! found_symbol ) {
    // get hash from name
    uint32_t hash = current->hash.build( name );
    uint32_t nbucket = current->hash.nbucket;
    uint32_t nchain = current->hash.nchain;
    uint32_t* buckets = &current->hash.table[ 2 ];
    uint32_t* chain = &current->hash.table[ 2 + nbucket ];
    Elf32_Sym* sym;
    const char* sym_name;
    uint32_t index = buckets[ hash % nbucket ];
    while ( index != STN_UNDEF) {
      if ( index > nchain) {
        // FIXME: SET ERROR AND RETURN?
        break;
      }
      // get symbol
      sym = &current->symtab[ index ];
      if ( sym->st_name > current->strsz ) {
        // FIXME: SET ERROR AND RETURN?
        break;
      }
      sym_name = current->strtab + sym->st_name;
      if ( 0 == strcmp( sym_name, name ) ) {
        if ( sym->st_shndx != SHN_UNDEF ) {
          found_symbol = ( void* )sym->st_value;
          if ( current->relocated ) {
            found_symbol = ( void* )( current->memory_start + ( uint32_t )found_symbol );
          }
          break;
        } else {
          // undefined symbol
          break;
        }
      }
      index = chain[ index ];
    }
    // next handle
    current = current->next;
  }
  // return found symbol
  return found_symbol;
}

/**
 * @fn void dlsym*(void* restrict, const char* restrict)
 * @brief Get symbol of handle by name
 *
 * @param handle
 * @param name
 *
 * @todo return symbol address instead of symbol
 */
void* dlsym( void* __restrict handle, const char* __restrict name ) {
  return dl_lookup_symbol( handle, name );
}

/**
 * @fn dl_image_handle_ptr_t dl_find_loaded_library(const char*)
 * @brief Helper to find library by file name
 *
 * @param file
 * @return
 */
dl_image_handle_ptr_t dl_find_loaded_library( const char* file ) {
  dl_image_handle_ptr_t current = root_object_handle;
  // loop objects
  while ( current ) {
    // return if matching
    if ( 0 == strcmp( current->filename, file ) ) {
      return current;
    }
    // next entry
    current = current->next;
  }
  // will be null if not found
  return current;
}

/**
 * @fn int dl_lookup_library(char*, size_t, const char*)
 * @brief Lookup library in paths
 *
 * @param buffer
 * @param buffer_size
 * @param file
 * @return
 *
 * @todo add file lookup by config and/or environment
 */
int dl_lookup_library( char* buffer, size_t buffer_size, const char* file ) {
  // build full path
  char* p = buffer;
  size_t sz = strlen( "/lib/" );
  // copy lib folder to buffer
  strncpy( p, "/lib/", buffer_size );
  buffer_size -= sz;
  p += sz;
  // attach file
  sz = strlen( file );
  strncpy( p, file, buffer_size );
  p += sz;
  buffer_size -= sz;
  // try to open
  return open( buffer, O_RDONLY );
}

/**
 * @fn dl_image_handle_ptr_t dl_allocate_handle(void)
 * @brief Helper to allocate new handle
 *
 * @return
 */
dl_image_handle_ptr_t dl_allocate_handle( void ) {
  // allocate new handle
  dl_image_handle_ptr_t handle = malloc( sizeof( dl_image_handle_t ) );
  // handle error
  if ( ! handle ) {
    return NULL;
  }
  // clear handle
  memset( handle, 0, sizeof( dl_image_handle_t ) );
  // attach to root object if existing
  if ( root_object_handle ) {
    // get end of list
    dl_image_handle_ptr_t current = root_object_handle;
    while ( current->next ) {
      current = current->next;
    }
    // populate next of current
    current->next = handle;
    // populate handle previous pointer
    handle->previous = current;
  // set root object if not existing
  } else {
    root_object_handle = handle;
  }
  // return created handle
  return handle;
}

/**
 * @fn void dl_free_handle(dl_image_handle_ptr_t)
 * @brief Helper to free handle
 *
 * @param handle
 */
void dl_free_handle( dl_image_handle_ptr_t handle ) {
  // stop if null
  if ( ! handle ) {
    return;
  }

  // handle free of root handle
  if ( root_object_handle == handle ) {
    root_object_handle = handle->next;
  }
  // set previous of next if existing
  if ( handle->next ) {
    handle->next->previous = handle->previous;
  }
  // set next of previous if existing
  if ( handle->previous ) {
    handle->previous->next = handle->next;
  }
  // free inner allocations
  free( handle->filename );
  // close descriptor
  close( handle->descriptor );
  // free handle itself
  free( handle );
}

/**
 * @fn dl_image_handle_ptr_t dl_load_dependency(dl_image_handle_ptr_t)
 * @brief Helper to load dependency of a handle
 *
 * @param handle
 * @return
 */
dl_image_handle_ptr_t dl_load_dependency( dl_image_handle_ptr_t handle ) {
  // in case it's invalid or no dynamic handles have been passed
  if ( ! handle || ! handle->dyn ) {
    return handle;
  }

  // load needed images
  for ( Elf32_Dyn* current = handle->dyn; current->d_tag != DT_NULL; current++ ) {
    // skip everything except DT_NEEDED
    if ( DT_NEEDED != current->d_tag ) {
      continue;
    }
    // string table offset for lib name
    Elf32_Word libname_offset = current->d_un.d_val;
    char* name = handle->strtab + libname_offset;
    // open with same mode options
    dl_image_handle_ptr_t dependency = dlopen(
      name,
      handle->open_mode
    );
    if ( ! dependency ) {
      dl_image_handle_ptr_t current_handle = root_object_handle;
      while ( current_handle ) {
        dl_free_handle( current_handle );
        current_handle = current_handle->next;
      }
      return NULL;
    }
  }
  return handle;
}

/**
 * @fn void dl_map_load_section*(void*, size_t, int, int, off_t)
 * @brief Helper to map a loadable section
 *
 * @param base
 * @param size
 * @param flag
 * @param descriptor
 * @param offset
 */
void* dl_map_load_section(
  void* base,
  size_t size,
  Elf32_Word flag,
  int descriptor,
  off_t offset
) {
  int prot = 0;
  if ( flag & PF_R ) {
    prot |= PROT_READ;
  }
  if ( flag & PF_W ) {
    prot |= PROT_WRITE;
  }
  if ( flag & PF_X ) {
    prot |= PROT_EXEC;
  }
  // mapping flags
  int mapping = MAP_PRIVATE;
  if ( base ) {
    mapping |= MAP_FIXED;
  }
  // call to mmap
  return mmap( base, size, prot, mapping, descriptor, offset );
}

/**
 * @fn dl_image_handle_ptr_t dl_load_entry(const char*, int, int)
 * @brief Helper to load handle
 *
 * @param file
 * @param mode
 * @param descriptor
 * @return
 */
dl_image_handle_ptr_t dl_load_entry(
  const char* file,
  int mode,
  int descriptor
) {
  // variables
  Elf32_Phdr program_header;
  ssize_t r;
  off_t offset;
  // create handle
  dl_image_handle_ptr_t handle = dl_allocate_handle();
  if ( ! handle ) {
    _dl_error = E_DL_NO_MEMORY;
    return NULL;
  }
  // read elf header
  r = read( descriptor, &handle->header, sizeof( handle->header ) );
  // handle error
  if ( r != sizeof( handle->header ) ) {
    _dl_error = E_DL_READ_HDR;
    dl_free_handle( handle );
    return NULL;
  }

  // check for gzip compression and load whole file if so
  uint8_t* compression = ( uint8_t* )&handle->header;
  // check if no gzip was used
  if ( 0x1f == compression[ 0 ] && 0x8b == compression[ 1 ] ) {
    _dl_error = E_DL_COMPRESSED_NOT_SUPPORTED;
    dl_free_handle( handle );
    return NULL;
  }

  // check magic
  if (
    ELFMAG0 != handle->header.e_ident[ EI_MAG0 ]
    || ELFMAG1 != handle->header.e_ident[ EI_MAG1 ]
    || ELFMAG2 != handle->header.e_ident[ EI_MAG2 ]
    || ELFMAG3 != handle->header.e_ident[ EI_MAG3 ]
  ) {
    _dl_error = E_DL_HEADER_VALIDATE_MAGIC;
    dl_free_handle( handle );
    return NULL;
  }
  // check architecture
  if ( ELFCLASS32 != handle->header.e_ident[ EI_CLASS ] ) {
    _dl_error = E_DL_HEADER_VALIDATE_BYTE;
    dl_free_handle( handle );
    return NULL;
  }
  if ( EM_ARM != handle->header.e_machine ) {
    _dl_error = E_DL_HEADER_VALIDATE_MACHINE;
    dl_free_handle( handle );
    return NULL;
  }
  // further header checks
  if ( 0 == handle->header.e_phentsize ) {
    _dl_error = E_DL_HEADER_VALIDATE_PROGRAM;
    dl_free_handle( handle );
    return NULL;
  }
  // ensure section header existence
  if ( 0 == handle->header.e_shentsize ) {
    _dl_error = E_DL_HEADER_VALIDATE_SECTION;
    dl_free_handle( handle );
    return NULL;
  }
  // allocate name
  handle->filename = strdup( file );
  if ( ! handle->filename ) {
    _dl_error = E_DL_NO_MEMORY;
    dl_free_handle( handle );
    return NULL;
  }
  // populate handle attributes
  handle->open_mode = mode;
  handle->descriptor = descriptor;
  handle->link_count = 1;

  // loop through program header count and save them
  size_t phdr_load_count = 0;
  for ( uint32_t idx = 0; idx < handle->header.e_phnum; idx++ ) {
    // calculate inner file offset
    offset = ( off_t )(
      handle->header.e_phoff + handle->header.e_phentsize * idx
    );
    // set file pointer to offset
    offset = lseek( handle->descriptor, offset, SEEK_SET );
    if ( -1 == offset ) {
      _dl_error = E_DL_IO_ERROR;
      dl_free_handle( handle );
      return NULL;
    }
    // try to read
    r = read( handle->descriptor, &program_header, sizeof( Elf32_Phdr ) );
    if ( r != sizeof( Elf32_Phdr ) ) {
      _dl_error = E_DL_IO_ERROR;
      dl_free_handle( handle );
      return NULL;
    }
    // reallocate phdr of handle
    handle->phdr = realloc( handle->phdr, ( idx + 1 ) * sizeof( Elf32_Phdr ) );
    if ( ! handle->phdr ) {
      _dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    // copy header
    memcpy( &handle->phdr[ idx ], &program_header, sizeof( Elf32_Phdr ) );
    // increase load count if of type load
    if ( PT_LOAD == program_header.p_type ) {
      phdr_load_count++;
    }
  }

  // populate load header array and cache temporarily dynamic section
  Elf32_Phdr* load_header = malloc( phdr_load_count * sizeof( Elf32_Phdr ) );
  Elf32_Phdr* dyn = NULL;
  for ( uint32_t idx = 0, load_idx = 0; idx < handle->header.e_phnum; idx++ ) {
    if ( PT_LOAD == handle->phdr[ idx ].p_type ) {
      load_header[ load_idx++ ] = handle->phdr[ idx ];
    }
    if ( PT_DYNAMIC == handle->phdr[ idx ].p_type ) {
      dyn = &handle->phdr[ idx ];
    }
  }

  // Load headers ( either one or two )
  char *memory = NULL;
  char *data = NULL;
  if ( 1 == phdr_load_count ) {
    offset = ( off_t )ROUND_DOWN_TO_FULL_PAGE( load_header[ 0 ].p_offset );
    size_t length = ROUND_UP_TO_FULL_PAGE( load_header[ 0 ].p_memsz + ROUND_PAGE_OFFSET( load_header[ 0 ].p_offset ) );
    // map loadable section
    memory = dl_map_load_section(
      ( void* )load_header[ 0 ].p_vaddr,
      length,
      load_header[ 0 ].p_flags,
      handle->descriptor,
      offset
    );
    if ( memory == MAP_FAILED ) {
      _dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    // populate memory start and length
    handle->memory_start = memory;
    handle->memory_size = length;
    handle->relocated = ( Elf32_Addr )memory != load_header[ 0 ].p_vaddr;
    // populate dynamic address
    if ( dyn ) {
      handle->dyn = ( Elf32_Dyn* )(
        ( char* )memory + dyn->p_vaddr - load_header[ 0 ].p_vaddr
      );
    }
  } else if ( 2 == phdr_load_count ) {
    uintptr_t text_address = ROUND_DOWN_TO_FULL_PAGE( load_header[ 0 ].p_vaddr );
    off_t text_offset = ( off_t )ROUND_DOWN_TO_FULL_PAGE( load_header[ 0 ].p_offset );
    off_t text_off = ROUND_PAGE_OFFSET( load_header[ 0 ].p_offset );
    size_t text_size = ROUND_UP_TO_FULL_PAGE( load_header[ 0 ].p_memsz + ( size_t )text_off );

    uintptr_t data_address = ROUND_DOWN_TO_FULL_PAGE( load_header[ 1 ].p_vaddr );
    off_t data_offset = ( off_t )ROUND_DOWN_TO_FULL_PAGE( load_header[ 1 ].p_offset  );
    off_t data_off = ROUND_PAGE_OFFSET( load_header[ 1 ].p_offset );
    size_t data_size = ROUND_UP_TO_FULL_PAGE( load_header[ 1 ].p_memsz + ( size_t )data_off );
    size_t data_file_size = ROUND_UP_TO_FULL_PAGE( load_header[ 1 ].p_filesz + ( size_t )data_off );
    // map text section with data size to get everything loaded correctly
    memory = dl_map_load_section(
      ( void* )text_address,
      text_size + data_size,
      load_header[ 0 ].p_flags,
      handle->descriptor,
      text_offset
    );
    if ( memory == MAP_FAILED ) {
      _dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    // release previously probably wrong data section mapping
    munmap( memory + data_address - text_address, data_size );
    // map data section again with only file size
    data = dl_map_load_section(
      ( void* )( memory + data_address - text_address ),
      data_file_size,
      load_header[ 1 ].p_flags,
      handle->descriptor,
      data_offset
    );
    if ( data == MAP_FAILED ) {
      _dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    // map more space if necessary
    if ( data_size > data_file_size ) {
      char* bss = mmap(
        ( void* )( data + data_file_size ),
        data_size - data_file_size,
        PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS,
        -1,
        0
      );
      // handle error
      if ( bss == MAP_FAILED ) {
        _dl_error = E_DL_NO_MEMORY;
        dl_free_handle( handle );
        return NULL;
      }
    }

    // populate memory start and length
    handle->memory_start = memory;
    handle->memory_size = text_size + data_size;
    handle->relocated = ( Elf32_Addr )memory != load_header[ 0 ].p_vaddr;
    // set dynamic
    if ( dyn ) {
      handle->dyn = ( Elf32_Dyn* )( memory + dyn->p_vaddr - load_header[ 0 ].p_vaddr );
    }
  } else {
    _dl_error = E_DL_UNKNOWN;
    dl_free_handle( handle );
    return NULL;
  }

  // populate further stuff
  if ( handle->dyn ) {
    // necessary data for loading everything
    Elf32_Addr strtab = 0;
    Elf32_Word strsz = 0;
    Elf32_Dyn* current = handle->dyn;
    Elf32_Addr symtab = 0;
    Elf32_Addr hash = 0;
    dl_image_hash_style_t hash_style = 0;
    hash_callback_t build = NULL;
    Elf32_Addr jmprel = 0;
    Elf32_Word pltrel = 0;
    Elf32_Word pltrelsz = 0;
    Elf32_Addr rel = 0;
    Elf32_Word relsz = 0;
    Elf32_Word relent = 0;
    Elf32_Addr rela = 0;
    Elf32_Word relasz = 0;
    Elf32_Word relaent = 0;
    Elf32_Addr pltgot = 0;
    init_callback_t init = NULL;
    init_callback_t fini = NULL;/*
    init_callback_t init_array;
    Elf32_Word init_array_size = 0;
    init_callback_t fini_array;
    Elf32_Word fini_array_size = 0;*/

    // loop through dyn section entries and populate some necessary data
    while( current->d_tag != DT_NULL ) {
      switch( current->d_tag ) {
        case DT_STRTAB:
          strtab = ( uintptr_t )current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          break;
        case DT_STRSZ:
          strsz = current->d_un.d_val;
          break;
        case DT_HASH:
          hash = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          hash_style = DL_IMAGE_HASH_STYLE_SYSTEM_V;
          build = dl_elf_symbol_name_hash;
          break;
        case DT_GNU_HASH:
          hash = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          hash_style = DL_IMAGE_HASH_STYLE_GNU;
          build = dl_gnu_symbol_name_hash;
          break;
        case DT_SYMTAB:
          symtab = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          break;
        case DT_JMPREL:
          jmprel = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          break;
        case DT_PLTREL:
          pltrel = current->d_un.d_val;
          break;
        case DT_PLTRELSZ:
          pltrelsz = current->d_un.d_val;
          break;
        case DT_REL:
          rel = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          break;
        case DT_RELSZ:
          relsz = current->d_un.d_val;
          break;
        case DT_RELENT:
          relent = current->d_un.d_val;
          break;
        case DT_RELA:
          rela = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          break;
        case DT_RELASZ:
          relasz = current->d_un.d_val;
          break;
        case DT_RELAENT:
          relaent = current->d_un.d_val;
          break;
        case DT_PLTGOT:
          pltgot = current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr;
          break;
        case DT_INIT:
          init = ( init_callback_t )(
            current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr
          );
          break;
        case DT_FINI:
          fini = ( init_callback_t )(
            current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr
          );
          break;/*
        case DT_INIT_ARRAY:
          init_array = ( init_callback_t )(
            current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr
          );
          break;
        case DT_INIT_ARRAYSZ:
          init_array_size = current->d_un.d_val;
          break;
        case DT_FINI_ARRAY:
          fini_array = ( init_callback_t )(
            current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr
          );
          break;
        case DT_FINI_ARRAYSZ:
          fini_array_size = current->d_un.d_val;
          break;*/
      }
      // get to next one
      current++;
    }

    // validate string table
    if ( strsz > UINT32_MAX - strtab || strtab + strsz > handle->memory_size ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    // set local pointer for strtab and ensure termination
    char* str = ( char* )( handle->memory_start + strtab );
    if ( '\0' != str[ strsz - 1 ] ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    // push to handle structure
    handle->strtab = str;
    handle->strsz = strsz;

    // symbol table
    if ( symtab > handle->memory_size ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    handle->symtab = ( Elf32_Sym* )( handle->memory_start + symtab );

    // hash table
    if ( hash > handle->memory_size - 8 ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    handle->hash.table = ( uint32_t* )( handle->memory_start + hash );
    handle->hash.style = hash_style;
    handle->hash.nbucket = handle->hash.table[ 0 ];
    handle->hash.nchain = handle->hash.table[ 1 ];
    handle->hash.build = build;
    // validate
    if (
      handle->hash.nbucket > UINT32_MAX - handle->hash.nchain
      || handle->hash.nbucket + handle->hash.nchain > ( handle->memory_size - hash ) / 4
    ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }

    // jmprel
    if ( pltrelsz > UINT32_MAX - jmprel || jmprel + pltrelsz > handle->memory_size ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    if ( jmprel ) {
      handle->jmprel = ( void* )( handle->memory_start + jmprel );
      handle->pltrel = pltrel;
      handle->pltrelsz = pltrelsz;
    }

    // rel
    if ( relsz > UINT32_MAX - rel || rel + relsz > handle->memory_size ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    if ( rel ) {
      handle->rel = ( void* )( handle->memory_start + rel );
      handle->relsz = relsz;
      handle->relent = relent;
    }
    // rela
    if ( relasz > UINT32_MAX - rela || rela + relasz > handle->memory_size ) {
      // FIXME: SET CORRECT ERRNO
      _dl_error = E_DL_UNKNOWN;
      dl_free_handle( handle );
      return NULL;
    }
    if ( rela ) {
      handle->rela = ( void* )( handle->memory_start + rela );
      handle->relasz = relsz;
      handle->relaent = relaent;
    }
    // global object table
    if ( pltgot ) {
      handle->pltgot = ( void** )( handle->memory_start + pltgot );
    }
    // init / fini
    if ( init ) {
      uintptr_t tmp_init = ( uintptr_t )init + ( uintptr_t )handle->memory_start;
      handle->init = ( init_callback_t )tmp_init;
    }
    if ( fini ) {
      uintptr_t tmp_fini = ( uintptr_t )fini + ( uintptr_t )handle->memory_start;
      handle->fini = ( init_callback_t )tmp_fini;
    }
  }
  // free load header again
  free( load_header );
  // return with load of dependencies
  return dl_load_dependency( handle );
}

/**
 * @fn dl_image_handle_ptr_t dl_post_init(dl_image_handle_ptr_t)
 * @brief
 *
 * @param handle
 * @return
 */
dl_image_handle_ptr_t dl_post_init( dl_image_handle_ptr_t handle ) {
  // call for init if existing
  if ( handle->init ) {
    handle->init();
  }
  // return handle
  return handle;
}

/**
 * @fn void dl_resolve_lazy*(dl_image_handle_ptr_t, uint32_t)
 * @brief Internal method to resolve a symbol from global offset table on demand
 *
 * @param object
 * @param symbol
 */
void* dl_resolve_lazy( dl_image_handle_ptr_t handle, uint32_t offset ) {
  // get relocation entry
  Elf32_Rel *rel = ( Elf32_Rel* )( ( ( char* )handle->jmprel ) + offset );
  uint32_t symbol_index = ELF32_R_SYM( rel->r_info );
  // get symbol name and symbol by name
  char* symbol_name = handle->strtab + handle->symtab[ symbol_index ].st_name;
  void* symbol_value = dlsym( NULL, symbol_name );
  // handle no symbol found!
  if ( ! symbol_value ) {
    exit( 42 );
  }
  // replace symbol
  uint32_t* target = ( uint32_t* )rel->r_offset;
  if ( handle->relocated ) {
    target = ( uint32_t* )( handle->memory_start + ( uint32_t )target );
  }
  // adjust memory
  *target = ( uint32_t )symbol_value;
  // return found symbol value
  return symbol_value;
}

/**
 * @fn dl_image_handle_ptr_t dl_relocate(dl_image_handle_ptr_t)
 * @brief Relocate all symbols of handle
 *
 * @param handle
 * @return
 */
dl_image_handle_ptr_t dl_relocate( dl_image_handle_ptr_t handle ) {
  if ( ! ( handle->open_mode & RTLD_NOW ) ) {
    return handle;
  }

  // global offset table handling
  if ( handle->pltgot ) {
    // get address of handler
    uintptr_t handler = ( uintptr_t )dl_resolve_stub;
    // Relocate dynamic section in case handle is relocated
    if ( handle->relocated ) {
      handle->pltgot[ 0 ] = ( void* )(
        handle->memory_start + ( size_t )handle->pltgot[ 0 ] );
    }
    // push image address
    handle->pltgot[ 1 ] = handle;
    // push handler address
    handle->pltgot[ 2 ] = ( void* )handler;
  }

  if ( handle->jmprel ) {
    if ( DT_REL == handle->pltrel ) {
      // FIXME: Move to helper function
      for (
        Elf32_Rel* rel = ( Elf32_Rel* )handle->jmprel;
        rel < ( Elf32_Rel* )handle->jmprel + ( handle->pltrelsz / sizeof( *rel ) );
        rel++
      ) {
        if ( handle->open_mode & RTLD_NOW ) {
          uint32_t symbol_index = ELF32_R_SYM( rel->r_info );
          // get symbol name and symbol by name
          char* symbol_name = handle->strtab + handle->symtab[ symbol_index ].st_name;
          void* symbol_value = dlsym( NULL, symbol_name );
          // handle no symbol found!
          if ( ! symbol_value ) {
            continue;
          }
          // replace symbol
          uint32_t* target = ( uint32_t* )rel->r_offset;
          if ( handle->relocated ) {
            target = ( uint32_t* )( handle->memory_start + ( uint32_t )target );
          }
          // adjust memory
          *target = ( uint32_t )symbol_value;
        } else {
          // determine relocation address
          uint32_t* relocation = ( uint32_t* )rel->r_offset;
          if ( handle->relocated ) {
            relocation = ( uint32_t* )( handle->memory_start + rel->r_offset );
            *relocation += ( uint32_t )handle->memory_start;
          }
        }
      }
    } else if ( DT_RELA == handle->pltrel ) {
      // FIXME: ADD IF NECESSARY
      fprintf( stderr, "Support for DT_RELA not yet implemented!\r\n" );
      return NULL;
    }
  }

  if ( handle->rel ) {
    // FIXME: MOVE INTO HELPER FUNCTION
    for (
      Elf32_Rel* rel = ( Elf32_Rel* )handle->rel;
      rel < ( Elf32_Rel* )handle->rel + ( handle->relsz / handle->relent );
      rel++
    ) {
      uint32_t symbol_index = ELF32_R_SYM( rel->r_info );
      uint32_t symbol_type = ELF32_R_TYPE( rel->r_info );
      // get symbol name and symbol by name
      char* name = handle->strtab + handle->symtab[ symbol_index ].st_name;
      uintptr_t new_address = 0;
      // determine relocation address
      uint32_t* relocation = ( uint32_t* )rel->r_offset;
      if ( handle->relocated ) {
        relocation = ( uint32_t* )( handle->memory_start + rel->r_offset );
      }

      if ( R_ARM_NONE == symbol_type ) {
        new_address = *relocation;
      } else if ( R_ARM_COPY == symbol_type ) {
        size_t size = handle->symtab[ symbol_index ].st_size;
        void* from = dlsym( handle->next, name );
        memcpy( relocation, from, size );
      } else if ( R_ARM_GLOB_DAT == symbol_type ) {
        new_address = ( uintptr_t )dlsym( handle, name );
      } else if ( R_ARM_ABS32 == symbol_type ) {
        uint32_t val = handle->symtab[ symbol_type ].st_value;
        if ( val ) {
          new_address = ( uintptr_t )( handle->memory_start + val );
        } else {
          new_address = ( uintptr_t )dlsym( NULL, name );
        }
      } else if (
        R_ARM_JUMP_SLOT == symbol_type
        || R_ARM_RELATIVE == symbol_type
      ) {
        new_address = ( uintptr_t )( handle->memory_start + *relocation );
      } else {
        fprintf(
          stderr,
          "Unsupported relocation type ( rel %ld )\n",
          symbol_type
        );
        return NULL;
      }
      // set new address if set
      if ( R_ARM_COPY != symbol_type && 0 < new_address ) {
        *relocation = new_address;
      }
    }
  }
  if ( handle->rela ) {
    // FIXME: ADD IF NECESSARY
    fprintf( stderr, "Support for DT_RELA not yet implemented!\r\n" );
    return NULL;
  }

  // return with init after relocate
  return dl_post_init( handle );
}

/**
 * @fn void dlopen*(const char*, int)
 * @brief dlopen implementation
 *
 * @param file
 * @param mode
 *
 */
void* dlopen( const char* file, int mode ) {
  // set error location
  _dl_error_location = "dlopen";
  // some variables
  int fd;
  const char* p;
  // lookup file if path is given
  if ( file ) {
    // check for file path
    if ( '/' == file[ 0 ] ) {
      // open file
      fd = open( file, O_RDONLY );
      p = file;
    } else {
      fd = dl_lookup_library( dl_open_buffer, sizeof( dl_open_buffer ) - 1, file );
      p = dl_open_buffer;
    }
    // lookup loaded stuff
    dl_image_handle_ptr_t found = dl_find_loaded_library( p );
    // handle already loaded
    if ( found ) {
      // increase link count
      found->link_count += 1;
      // close again
      close( fd );
      // return object
      return found;
    }
    // error handling
    if ( -1 == fd ) {
      _dl_error = E_DL_CANNOT_OPEN;
      return NULL;
    }
    // load given handle with dependencies
    dl_image_handle_ptr_t handle = dl_load_entry( p, mode, fd );
    // handle error
    if ( ! handle ) {
      close( fd );
      return NULL;
    }
    // return with possible relocation
    return dl_relocate( handle );
  }
  // no file parameter? => return current root handle
  return root_object_handle;
}

int dlclose( void* handle ) {
  // set error location
  _dl_error_location = "dlclose";
  // skip invalid handles
  if ( ! handle ) {
    return 0;
  }
  // convert to dl handle
  dl_image_handle_ptr_t converted_handle = handle;
  // decrease link count
  converted_handle->link_count -= 1;
  // skip if still referenced somewhere
  if ( 0 < converted_handle->link_count ) {
    return 0;
  }

  // FIXME: CALL CURRENT HANDLE FINI FUNCTION IF EXISTING
  // FIXME: CALL CURRENT HANDLE FINI ARRAY FUNCTION IF EXISTING
  // FIXME: CLOSE REFERENCES USING CURRENT HANDLE
  // FIXME: UNMAP handle WITH MUNMAP

  // remove handle from list
  dl_free_handle( handle );
  // return success
  return 0;
}
