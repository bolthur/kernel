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

// FIXME: RESTRICT TO COMPILING NEWLIB ONLY IF NECESSARY

#include <stdbool.h>
#include <elf.h>
#include <dlfcn.h>

#if !defined( ___DL_INT_H__ )
#define ___DL_INT_H__

// FIXME: USE sysconf for page size once implemented
#define PAGE_SIZE 0x1000
#define ROUND_DOWN_TO_FULL_PAGE( a ) \
  ( ( uintptr_t )( a ) & ( uintptr_t )( ~( ( PAGE_SIZE ) - 1 ) ) )
#define ROUND_UP_TO_FULL_PAGE( a ) \
  ( ( ( ( uintptr_t )( a ) & ( ( PAGE_SIZE ) - 1 ) ) ? ( PAGE_SIZE ) : 0 ) \
    + ROUND_DOWN_TO_FULL_PAGE( ( a ) ) )
#define ROUND_PAGE_OFFSET( a ) ( ( uintptr_t )( a ) & ( ( PAGE_SIZE ) -1 ) )

typedef void ( *init_callback_t )( void );
typedef void ( **init_array_callback_t )( void );
typedef uint32_t ( *hash_callback_t )( const char* );
typedef void ( *main_entry_point )( void );

typedef enum {
  DL_IMAGE_HASH_STYLE_SYSTEM_V = 1,
  DL_IMAGE_HASH_STYLE_GNU = 2,
} dl_image_hash_style_t;

typedef struct dl_image_handle dl_image_handle_t;
typedef struct dl_image_handle* dl_image_handle_ptr_t;
struct dl_image_handle {
  // file header
  Elf32_Ehdr header;

  // pointer to next / previous entry
  dl_image_handle_ptr_t next;
  dl_image_handle_ptr_t previous;

  // file information
  int descriptor;
  int open_mode;
  int link_count;
  char* filename;
  bool relocated;

  // init and fini function reference
  init_callback_t init;
  init_callback_t fini;
  init_array_callback_t init_array;
  size_t init_array_size;
  init_array_callback_t fini_array;
  size_t fini_array_size;
  // more general information for linking / loading
  char* memory_start;
  size_t memory_size;

  // necessary elf section attributes
  Elf32_Dyn* dyn;
  Elf32_Phdr* phdr;
  char* strtab;
  size_t strsz;
  Elf32_Sym* symtab;
  struct {
    uint32_t* table;
    dl_image_hash_style_t style;
    uint32_t nbucket;
    uint32_t nchain;
    hash_callback_t build;
  } hash;
  void* jmprel;
  uint32_t pltrel;
  uint32_t pltrelsz;
  void* rel;
  uint32_t relsz;
  uint32_t relent;
  void* rela;
  uint32_t relasz;
  uint32_t relaent;
  void** pltgot;
};

dl_image_handle_ptr_t dl_find_loaded_library( const char* );
int dl_lookup_library( char*, size_t, const char* );
dl_image_handle_ptr_t dl_allocate_handle( void );
void dl_free_handle( dl_image_handle_ptr_t );
dl_image_handle_ptr_t dl_load_entry( const char*, int, int );
dl_image_handle_ptr_t dl_load_dependency( dl_image_handle_ptr_t );
void* dl_map_load_section( void*, size_t, Elf32_Word, int, off_t );
dl_image_handle_ptr_t dl_relocate( dl_image_handle_ptr_t );
void dl_handle_rel_symbol( dl_image_handle_ptr_t, void*, size_t );
bool dl_handle_rel_relocate( dl_image_handle_ptr_t, void*, size_t );
dl_image_handle_ptr_t dl_post_init( dl_image_handle_ptr_t );
void* dl_lookup_symbol( dl_image_handle_ptr_t, const char* );
void* dl_resolve_lazy( dl_image_handle_ptr_t, uint32_t );
uintptr_t dl_resolve_stub( void );

uint32_t dl_elf_symbol_name_hash( const char* );
uint32_t dl_gnu_symbol_name_hash( const char* );

#endif
