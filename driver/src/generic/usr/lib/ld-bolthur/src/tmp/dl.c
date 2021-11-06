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
#include <sys/bolthur.h>
#include "_dl-int.h"

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
#define E_DL_IO_ERROR 10 /* I/O ERROR */
#define E_DL_MALFORMED 11 /* Malformed ELF data */
#define E_DL_UNKNOWN_SYMBOL 12 /* Unknown symbol during relocation */
#define E_DL_DT_RELA_NOT_IMPLEMENTED 13 /* Relocation with DT_RELA not yet implemented */
#define E_DL_NOT_SUPPORTED 14 /* Not supported behaviour */
#define E_DL_UNKNOWN 15 /* Unknown error */

// internal structure containing single message
typedef struct {
  char* message;
} dl_message_entry_t;

static dl_message_entry_t dl_error_message[] = {
  { "Could not open library: " },
  { "Could not find library: " },
  { "Error while allocating memory" },
  { "Error while reading header" },
  { "Wrong elf magic / not an elf object" },
  { "Wrong elf byte class" },
  { "Wrong elf machine" },
  { "No program header found" },
  { "No section header found" },
  { "Malformed elf data" },
  { "Undefined not relocatable symbol" },
  { "Relocation using DT_RELA is not yet implemented" },
  { "Not supported behaviour" },
  { "Unknown error" },
};

// globals
// FIXME: PUSH VARIABLES TO REENT WHEN IMPORTING TO NEWLIB
uint32_t dl_error;
const char* dl_error_location;
const char* dl_error_data;
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
  if ( 0 == dl_error ) {
    strncpy( buffer, "no error", total_length );
    // reset globals
    dl_error_location = 0;
    dl_error_data = "";
    dl_error = 0;
    // return buffer
    return buffer;
  }
  // clear buffer
  memset( buffer, 0, sizeof( buffer ) );
  // determine entry count
  size_t error_count = sizeof( dl_error_message ) / sizeof( dl_message_entry_t );
  // handle invalid error code
  if ( dl_error >= error_count ) {
    // copy over last error message
    strncpy(
      buffer,
      dl_error_message[ error_count - 1 ].message,
      total_length
    );
    // reset globals
    dl_error_location = 0;
    dl_error_data = "";
    dl_error = 0;
    // return buffer
    return buffer;
  }
  // valid error code fill buffer
  char *buffer_pos = buffer;
  size_t length;
  // check if location is set
  if ( dl_error_location ) {
    // determine length of error location
    length = strlen( dl_error_location );
    // push location string to buffer
    strncpy( buffer_pos, dl_error_location, total_length );
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
  length = strlen( dl_error_message[ dl_error - 1 ].message );
  // push message string to buffer
  strncpy( buffer_pos, dl_error_message[ dl_error - 1 ].message, total_length );
  // decrement total length and increment buffer position
  total_length -= length;
  buffer_pos += length;
  // push error data if set
  if ( dl_error_data ) {
    strncpy( buffer_pos, dl_error_data, total_length );
  }
  // reset globals
  dl_error_location = 0;
  dl_error_data = "";
  dl_error = 0;
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
 */
uint32_t dl_gnu_symbol_name_hash( const char* name ) {
  uint32_t h = 5381;
  while( *name ) {
    h = ( h << 5 ) + h + *name++;
  }
  return h;
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
    uint32_t index = buckets[ hash % nbucket ];
    while ( index != STN_UNDEF) {
      // skip if wrong
      if ( index > nchain) {
        break;
      }
      // get symbol
      Elf32_Sym* sym = &current->symtab[ index ];
      // skip if wrong
      if ( sym->st_name > current->strsz ) {
        break;
      }
      // get symbol name
      const char* sym_name = current->strtab + sym->st_name;
      // check for matching symbol
      if ( 0 == strcmp( sym_name, name ) ) {
        // check for not undefined
        if ( sym->st_shndx != SHN_UNDEF ) {
          // set found symbol to value
          found_symbol = ( void* )sym->st_value;
          // apply relocation offset if relocated
          if ( current->relocated ) {
            found_symbol = ( void* )( current->memory_start
              + ( uint32_t )found_symbol );
          }
          break;
        } else {
          // undefined symbol
          break;
        }
      }
      // next index
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
 * @todo add file lookup within DT_RPATH somehow if set within original executable
 * @todo add file lookup by environment
 * @todo add file lookup by config file
 * @todo use /lib:/usr/lib:/ramdisk/lib:/ramdisk/usr/lib as fallback
 * @todo check whether it was loaded to shared area and use that one
 */
int dl_lookup_library( char* buffer, size_t buffer_size, const char* file ) {
  char* path = "/lib:/usr/lib:/ramdisk/lib:/ramdisk/usr/lib";
  char* part = NULL;
  char* last_part = NULL;
  for (
    part = strtok_r( path, ":", &last_part );
    part;
    part = strtok_r( NULL, ":", &last_part )
  ) {
    char* p = buffer;
    size_t size_part = strlen( part );
    size_t size_file = strlen( file );
    size_t buffer_size_backup = buffer_size;
    // copy first part of path
    strncpy( p, part, buffer_size_backup );
    buffer_size_backup -= size_part;
    p += size_part;
    // path separator
    strncpy( p, "/", buffer_size_backup );
    buffer_size_backup--;
    p++;
    // copy file
    strncpy( p, file, buffer_size_backup );
    buffer_size_backup -= size_file;
    p += size_file;
    // try to open
    int fd = open( buffer, O_RDONLY );
    // stop if found
    if ( -1 != fd ) {
      return fd;
    }
  }
  return -1;
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
  // unmap without success check
  if ( handle->memory_start ) {
    munmap( handle->memory_start, handle->memory_size );
  }
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
        // cache current handle and set current to next
        dl_image_handle_ptr_t tmp = current_handle;
        current_handle = current_handle->next;
        // free cached
        dl_free_handle( tmp );
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
    EARLY_STARTUP_PRINT( "Read\r\n" )
  }
  if ( flag & PF_W ) {
    prot |= PROT_WRITE;
    EARLY_STARTUP_PRINT( "Write\r\n" )
  }
  if ( flag & PF_X ) {
    prot |= PROT_EXEC;
    EARLY_STARTUP_PRINT( "Executable\r\n" )
  }
  // mapping flags
  int mapping = MAP_PRIVATE;
  if ( base ) {
    mapping |= MAP_FIXED;
  }
  EARLY_STARTUP_PRINT(
    "mmap( %p, %#zx, %#x, %#x, %#x, %#lx )\r\n",
    base, size, prot, mapping, descriptor, offset
  )
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
 *
 * @todo map library read-only code into shared object
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
    dl_error = E_DL_NO_MEMORY;
    return NULL;
  }
  // read elf header
  r = read( descriptor, &handle->header, sizeof( handle->header ) );
  // handle error
  if ( r != sizeof( handle->header ) ) {
    dl_error = E_DL_READ_HDR;
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
    dl_error = E_DL_HEADER_VALIDATE_MAGIC;
    dl_free_handle( handle );
    return NULL;
  }
  // check architecture
  if ( ELFCLASS32 != handle->header.e_ident[ EI_CLASS ] ) {
    dl_error = E_DL_HEADER_VALIDATE_BYTE;
    dl_free_handle( handle );
    return NULL;
  }
  if ( EM_ARM != handle->header.e_machine ) {
    dl_error = E_DL_HEADER_VALIDATE_MACHINE;
    dl_free_handle( handle );
    return NULL;
  }
  // further header checks
  if ( 0 == handle->header.e_phentsize ) {
    dl_error = E_DL_HEADER_VALIDATE_PROGRAM;
    dl_free_handle( handle );
    return NULL;
  }
  // ensure section header existence
  if ( 0 == handle->header.e_shentsize ) {
    dl_error = E_DL_HEADER_VALIDATE_SECTION;
    dl_free_handle( handle );
    return NULL;
  }
  // allocate name
  handle->filename = strdup( file );
  if ( ! handle->filename ) {
    dl_error = E_DL_NO_MEMORY;
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
      dl_error = E_DL_IO_ERROR;
      dl_free_handle( handle );
      return NULL;
    }
    // try to read
    r = read( handle->descriptor, &program_header, sizeof( Elf32_Phdr ) );
    if ( r != sizeof( Elf32_Phdr ) ) {
      dl_error = E_DL_IO_ERROR;
      dl_free_handle( handle );
      return NULL;
    }
    // reallocate phdr of handle
    handle->phdr = realloc( handle->phdr, ( idx + 1 ) * sizeof( Elf32_Phdr ) );
    if ( ! handle->phdr ) {
      dl_error = E_DL_NO_MEMORY;
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
  char* memory = NULL;
  char* data = NULL;
  if ( 1 == phdr_load_count ) {
    offset = ( off_t )ROUND_DOWN_TO_FULL_PAGE( load_header[ 0 ].p_offset );
    // FIXME: MAP SECTION IN CASE OF DEPENDENCY INTO SHARED AREA
    // map loadable section
    memory = dl_map_load_section(
      ( void* )load_header[ 0 ].p_vaddr,
      load_header[ 0 ].p_memsz,
      load_header[ 0 ].p_flags,
      handle->descriptor,
      offset
    );
    if ( memory == MAP_FAILED ) {
      dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    EARLY_STARTUP_PRINT( "loading to %#x with length %#lx\r\n",
      ( uintptr_t )memory,
      load_header[ 0 ].p_memsz )
    // populate memory start and length
    handle->memory_start = memory;
    handle->memory_size = load_header[ 0 ].p_memsz;
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
    EARLY_STARTUP_PRINT(
      "text_address = %#"PRIxPTR", text_offset = %#lx, text_off = %#lx, text_size = %#zx\r\n",
      text_address, text_offset, text_off, text_size
    )
    EARLY_STARTUP_PRINT(
      "load_header[ 0 ].p_vaddr = %#lx, load_header[ 0 ].p_offset = %#lx, "
      "load_header[ 0 ].p_memsz = %#lx, load_header[ 0 ].p_filesz = %#lx\r\n",
      load_header[ 0 ].p_vaddr, load_header[ 0 ].p_offset,
      load_header[ 0 ].p_memsz, load_header[ 0 ].p_filesz
    )

    uintptr_t data_address = ROUND_DOWN_TO_FULL_PAGE( load_header[ 1 ].p_vaddr );
    off_t data_offset = ( off_t )ROUND_DOWN_TO_FULL_PAGE( load_header[ 1 ].p_offset  );
    off_t data_off = ROUND_PAGE_OFFSET( load_header[ 1 ].p_offset );
    size_t data_size = ROUND_UP_TO_FULL_PAGE( load_header[ 1 ].p_memsz + ( size_t )data_off );
    size_t data_file_size = ROUND_UP_TO_FULL_PAGE( load_header[ 1 ].p_filesz + ( size_t )data_off );
    EARLY_STARTUP_PRINT(
      "data_address = %#"PRIxPTR", data_offset = %#lx, data_off = %#lx, "
      "data_size = %#zx, data_file_size = %#zx\r\n",
      data_address, data_offset, data_off, data_size, data_file_size
    )
    EARLY_STARTUP_PRINT(
      "load_header[ 1 ].p_vaddr = %#lx, load_header[ 1 ].p_offset = %#lx, "
      "load_header[ 1 ].p_memsz = %#lx, load_header[ 1 ].p_filesz = %#lx\r\n",
      load_header[ 1 ].p_vaddr, load_header[ 1 ].p_offset,
      load_header[ 1 ].p_memsz, load_header[ 1 ].p_filesz
    )

    // map text section with data size to get everything loaded correctly
    // FIXME: MAP SECTION IN CASE OF DEPENDENCY INTO SHARED AREA
    memory = dl_map_load_section(
      ( void* )text_address,
      text_size,
      load_header[ 0 ].p_flags,
      handle->descriptor,
      text_offset
    );
    if ( memory == MAP_FAILED ) {
      dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    EARLY_STARTUP_PRINT(
      "loaded to %#x with length %#zx. File offset: %#lx, data = %p\r\n",
      ( uintptr_t )memory, text_size, text_offset,
      ( void* )( memory + load_header[ 1 ].p_vaddr - load_header[ 0 ].p_vaddr )
    )
    // map data section again with only file size
    // FIXME: MAP SECTION IN CASE OF DEPENDENCY INTO SHARED AREA
    data = dl_map_load_section(
      ( void* )( memory + data_address - text_address ),
      data_file_size,
      load_header[ 1 ].p_flags,
      handle->descriptor,
      data_offset
    );
    if ( data == MAP_FAILED ) {
      dl_error = E_DL_NO_MEMORY;
      dl_free_handle( handle );
      return NULL;
    }
    size_t len = ( size_t )data_off + load_header[ 1 ].p_filesz;
    memset( data + len, 0, data_file_size - len );
    void* tmp_data = ( void* )( memory + load_header[ 1 ].p_vaddr - load_header[ 0 ].p_vaddr );
    EARLY_STARTUP_PRINT( "data = %p, *data = %#lx\r\n", tmp_data, *( ( uint32_t* )tmp_data ) )
    EARLY_STARTUP_PRINT( "loaded to %#x with length %#zx. File offset: %#lx\r\n",
      ( uintptr_t )data, data_file_size, data_offset )
    // map more space if necessary
    if ( data_size > data_file_size ) {
      // calculate bss start and length
      void* bss_start = ( void* )( data + data_file_size );
      char* bss = ( char* )bss_start;
      len = data_size - data_file_size;
      // debug output
      EARLY_STARTUP_PRINT(
        "requesting more size for bss section. "
        "data_size = %#zx, data_file_size = %#zx, "
        "len = %#zx, bss_start = %p\r\n",
        data_size, data_file_size, len, bss_start
      )
      // acquire via mmap
      // FIXME: MAP SECTION IN CASE OF DEPENDENCY INTO SHARED AREA
      bss = mmap(
        bss_start,
        len,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
        -1,
        0
      );
      // handle error
      if ( bss == MAP_FAILED ) {
        dl_error = E_DL_NO_MEMORY;
        dl_free_handle( handle );
        return NULL;
      }
      EARLY_STARTUP_PRINT(
        "Allocated space of %#zx at address %p.\r\n",
        len,
        ( void* )bss
      )
      // clear
      memset( ( void* )bss, 0, len );
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
    dl_error = E_DL_UNKNOWN;
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
    init_callback_t fini = NULL;
    init_array_callback_t init_array = NULL;
    Elf32_Word init_array_size = 0;
    init_array_callback_t fini_array = NULL;
    Elf32_Word fini_array_size = 0;

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
          break;
        case DT_INIT_ARRAY:
          init_array = ( init_array_callback_t )(
            current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr
          );
          break;
        case DT_INIT_ARRAYSZ:
          init_array_size = current->d_un.d_val / sizeof( uintptr_t );
          break;
        case DT_FINI_ARRAY:
          fini_array = ( init_array_callback_t )(
            current->d_un.d_ptr - ( uintptr_t )load_header[ 0 ].p_vaddr
          );
          break;
        case DT_FINI_ARRAYSZ:
          fini_array_size = current->d_un.d_val / sizeof( uintptr_t );
          break;
      }
      // get to next one
      current++;
    }

    // validate string table
    if ( strsz > UINT32_MAX - strtab || strtab + strsz > handle->memory_size ) {
      dl_error = E_DL_MALFORMED;
      dl_free_handle( handle );
      return NULL;
    }
    // set local pointer for strtab and ensure termination
    char* str = ( char* )( handle->memory_start + strtab );
    if ( '\0' != str[ strsz - 1 ] ) {
      dl_error = E_DL_MALFORMED;
      dl_free_handle( handle );
      return NULL;
    }
    // push to handle structure
    handle->strtab = str;
    handle->strsz = strsz;

    // symbol table
    if ( symtab > handle->memory_size ) {
      dl_error = E_DL_MALFORMED;
      dl_free_handle( handle );
      return NULL;
    }
    handle->symtab = ( Elf32_Sym* )( handle->memory_start + symtab );

    // hash table
    if ( hash > handle->memory_size - 8 ) {
      dl_error = E_DL_MALFORMED;
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
      dl_error = E_DL_MALFORMED;
      dl_free_handle( handle );
      return NULL;
    }

    // jmprel
    if ( pltrelsz > UINT32_MAX - jmprel || jmprel + pltrelsz > handle->memory_size ) {
      dl_error = E_DL_MALFORMED;
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
      dl_error = E_DL_MALFORMED;
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
      dl_error = E_DL_MALFORMED;
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
    // init_array / fini_array
    if ( init_array ) {
      uintptr_t tmp_init_array = ( uintptr_t )init_array + ( uintptr_t )handle->memory_start;
      handle->init_array = ( init_array_callback_t )tmp_init_array;
      handle->init_array_size = init_array_size;
    }
    if ( fini_array ) {
      uintptr_t tmp_fini_array = ( uintptr_t )fini_array + ( uintptr_t )handle->memory_start;
      handle->fini_array = ( init_array_callback_t )tmp_fini_array;
      handle->fini_array_size = fini_array_size;
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
    EARLY_STARTUP_PRINT( "Calling init of %s\r\n", handle->filename )
    handle->init();
  }
  // handle existing init array
  if ( handle->init_array ) {
    for( size_t idx = 0; idx < handle->init_array_size; idx++ ) {
      EARLY_STARTUP_PRINT(
        "Calling init array %zu of %s\r\n",
        idx, handle->filename )
      handle->init_array[ idx ]();
    }
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
 * @fn void dl_handle_rel_symbol(dl_image_handle_ptr_t, void*, size_t)
 * @brief Helper to handle global offset table relocations
 *
 * @param handle
 * @param address
 * @param size
 */
void dl_handle_rel_symbol( dl_image_handle_ptr_t handle, void* address, size_t size ) {
  for (
    Elf32_Rel* rel = ( Elf32_Rel* )address;
    rel < ( Elf32_Rel* )address + size;
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
      /*EARLY_STARTUP_PRINT( "%s => %p = %#lx\r\n",
        symbol_name, ( void* )target, ( uint32_t )symbol_value )*/
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
}

/**
 * @fn bool dl_handle_rel_relocate(dl_image_handle_ptr_t, void*, size_t)
 * @brief Helper to handle normal rel relocations
 *
 * @param handle
 * @param address
 * @param size
 * @return
 */
bool dl_handle_rel_relocate( dl_image_handle_ptr_t handle, void* address, size_t size ) {
  for (
    Elf32_Rel* rel = ( Elf32_Rel* )address;
    rel < ( Elf32_Rel* )address + size;
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
      size_t copy_size = handle->symtab[ symbol_index ].st_size;
      void* from = dlsym( handle->next, name );
      memcpy( relocation, from, copy_size );
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
      dl_error = E_DL_UNKNOWN_SYMBOL;
      return false;
    }
    // set new address if set
    if ( R_ARM_COPY != symbol_type && 0 < new_address ) {
      *relocation = new_address;
    }
  }
  return true;
}

/**
 * @fn dl_image_handle_ptr_t dl_relocate(dl_image_handle_ptr_t)
 * @brief Relocate all symbols of handle
 *
 * @param handle
 * @return
 */
dl_image_handle_ptr_t dl_relocate( dl_image_handle_ptr_t handle ) {
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
  // jmprel symbols
  if ( handle->jmprel ) {
    if ( DT_REL == handle->pltrel ) {
      dl_handle_rel_symbol(
        handle,
        handle->jmprel,
        handle->pltrelsz / sizeof( Elf32_Rel )
      );
    } else if ( DT_RELA == handle->pltrel ) {
      dl_error = E_DL_DT_RELA_NOT_IMPLEMENTED;
      return NULL;
    }
  }
  // normal relocations
  if ( handle->rel ) {
    if ( ! dl_handle_rel_relocate(
      handle,
      handle->rel,
      handle->relsz / handle->relent
    ) ) {
      return NULL;
    }
  }
  if ( handle->rela ) {
    dl_error = E_DL_DT_RELA_NOT_IMPLEMENTED;
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
  dl_error_location = "dlopen";
  // lookup file if path is given
  if ( file ) {
    // some variables
    const char* p;
    int fd;
    // check for file path
    if ( '/' == file[ 0 ] ) {
      // open file
      fd = open( file, O_RDONLY );
      p = file;
    } else {
      fd = dl_lookup_library(
        dl_open_buffer,
        sizeof( dl_open_buffer ) - 1,
        file );
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
    if ( '/' == file[ 0 ] ) {
      EARLY_STARTUP_PRINT("Load %s\r\n", file )
    } else {
      EARLY_STARTUP_PRINT(
        "%s - loading %s\r\n",
        root_object_handle->filename,
        dl_open_buffer )
    }
    // error handling
    if ( -1 == fd ) {
      dl_error = E_DL_CANNOT_OPEN;
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

/**
 * @fn int dl_close(dl_image_handle_ptr_t)
 * @brief Internal close function with correct type
 *
 * @param handle
 * @return
 */
static int dl_close( dl_image_handle_ptr_t handle ) {
  // decrease link count
  handle->link_count -= 1;
  // skip if still referenced somewhere
  if ( 0 < handle->link_count ) {
    return 0;
  }
  // execute fini
  if ( handle->fini ) {
    EARLY_STARTUP_PRINT( "Calling fini of %s\r\n", handle->filename )
    handle->fini();
  }
  // execute fini array
  if ( handle->fini_array ) {
    for( size_t idx = 0; idx < handle->fini_array_size; idx++ ) {
      EARLY_STARTUP_PRINT(
        "Calling fini array %zu of %s\r\n",
        idx, handle->filename )
      handle->fini_array[ idx ]();
    }
  }

  // decrement link count of libraries needed by this one
  for (
    Elf32_Dyn* current = handle->dyn;
    current->d_tag != DT_NULL;
    current++
  ) {
    // skip everything except DT_NEEDED
    if ( DT_NEEDED != current->d_tag ) {
      continue;
    }
    // get library name
    char* name = handle->strtab + current->d_un.d_val;
    // try to close library ( maybe a dependency )
    dlclose( dl_find_loaded_library( name ) );
  }

  // unmap memory
  if ( -1 == munmap( handle->memory_start, handle->memory_size ) ) {
    return -1;
  }
  // remove handle from list
  dl_free_handle( handle );
  // return success
  return 0;
}

/**
 * @fn int dlclose(void*)
 * @brief Close opened handle again
 *
 * @param handle
 * @return
 */
int dlclose( void* handle ) {
  // set error location
  dl_error_location = "dlclose";
  // skip invalid handles
  if ( ! handle ) {
    return 0;
  }
  return dl_close( handle );
}
