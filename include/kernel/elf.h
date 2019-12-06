
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

#if ! defined( __KERNEL_ELF__ )
#define __KERNEL_ELF__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  ELF_HEADER_MACHINE_NONE = 0,
  ELF_HEADER_MACHINE_ARM = 0x28,
  ELF_HEADER_MACHINE_AARCH64 = 0xB7,
} elf_header_machine_t;

typedef enum {
  ELF_HEADER_TYPE_NONE = 0,
  ELF_HEADER_TYPE_RELOCATABLE = 1,
  ELF_HEADER_TYPE_EXECUTABLE = 2,
  ELF_HEADER_TYPE_SHARED = 3,
  ELF_HEADER_TYPE_CORE = 4,
} elf_header_type_t;

typedef enum {
  ELF_HEADER_ENCODING_NONE = 0,
  ELF_HEADER_ENCODING_LITTLE_ENDIAN = 1,
  ELF_HEADER_ENCODING_BIG_ENDIAN = 2,
} elf_header_encoding_t;

typedef enum {
  ELF_HEADER_ARCHITECTURE_NONE = 0,
  ELF_HEADER_ARCHITECTURE_32 = 1,
  ELF_HEADER_ARCHITECTURE_64 = 2,
} elf_header_architecture_t;

typedef enum {
  ELF_HEADER_MAGIC_1 = 0x7f,
  ELF_HEADER_MAGIC_2 = 'E',
  ELF_HEADER_MAGIC_3 = 'L',
  ELF_HEADER_MAGIC_4 = 'F',
} elf_header_magic_t;

typedef struct {
  uint8_t magic[ 4 ];
  uint8_t architecture;
  uint8_t encoding;
  uint8_t header_version;
  uint8_t os_abi;
  uint8_t abi_version;
  uint8_t reserved[ 7 ];
  uint16_t type;
  uint16_t machine;
  uint32_t elf_version;
  uint32_t program_entry;
  uint32_t program_header;
  uint32_t section_header;
  uint32_t flags;
  uint16_t header_size;
  uint16_t program_header_size;
  uint16_t program_header_count;
  uint16_t section_header_size;
  uint16_t section_header_count;
  uint16_t section_header_index;
} elf_header_t, *elf_header_ptr_t;

typedef enum {
  ELF_PROGRAM_HEADER_TYPE_NONE = 0,
  ELF_PROGRAM_HEADER_TYPE_LOAD = 1,
  ELF_PROGRAM_HEADER_TYPE_DYNAMIC = 2
} elf_program_header_type_t;

typedef enum {
  ELF_PROGRAM_HEADER_FLAG_EXECUTABLE = 1,
  ELF_PROGRAM_HEADER_FLAG_WRITABLE = 2,
  ELF_PROGRAM_HEADER_FLAG_READABLE = 4
} elf_program_header_flags_t;

typedef struct {
  uint32_t type;
  uint32_t offset;
  uint32_t virtual;
  uint32_t physical;
  uint32_t file_size;
  uint32_t memory_size;
  uint32_t flags;
  uint32_t alignment;
} elf_program_header_t, *elf_program_header_ptr_t;

typedef enum {
  ELF_SECTION_HEADER_TYPE_NONE = 0,
  ELF_SECTION_HEADER_TYPE_PROGRAM = 1,
  ELF_SECTION_HEADER_TYPE_SYMBOL = 2,
  ELF_SECTION_HEADER_TYPE_STRING = 3,
  ELF_SECTION_HEADER_TYPE_RELOCATE_ADDEND = 4,
  ELF_SECTION_HEADER_TYPE_HASH = 5,
  ELF_SECTION_HEADER_TYPE_DYNAMIC = 6,
  ELF_SECTION_HEADER_TYPE_NOTE = 7,
  ELF_SECTION_HEADER_TYPE_NOT_PRESENT = 8,
  ELF_SECTION_HEADER_TYPE_RELOCATE_NO_ADDEND = 9,
} elf_section_header_type_t;

typedef enum {
  ELF_SECTION_HEADER_FLAG_WRITE = 1,
  ELF_SECTION_HEADER_FLAG_ALLOCATE = 2,
} elf_section_header_flag_t;

typedef struct {
  uint32_t name;
  uint32_t type;
  uint32_t flags;
  uint32_t address;
  uint32_t offset;
  uint32_t size;
  uint32_t link;
  uint32_t info;
  uint32_t align;
  uint32_t entry_size;
} elf_section_header_t, *elf_section_header_ptr_t;

typedef enum {
  ELF_SYMBOL_INFO_BINDING_LOCAL = 0,
  ELF_SYMBOL_INFO_BINDING_GLOBAL = 1,
  ELF_SYMBOL_INFO_BINDING_WEAK = 2,
} elf_symbol_info_binding_t;

typedef enum {
  ELF_SYMBOL_INFO_TYPE_NONE = 0,
  ELF_SYMBOL_INFO_TYPE_OBJECT = 1,
  ELF_SYMBOL_INFO_TYPE_FUNCTION = 2,
} elf_symbol_info_type_t;

typedef struct {
  uint32_t name;
  uint32_t value;
  uint32_t size;
  uint8_t info;
  uint8_t other;
  uint16_t secction_header_index;
} elf_symbol_t, *elf_symbol_ptr_t;

typedef struct {
  uint32_t offset;
  uint32_t info;
} elf_relocate_t, *elf_relocate_ptr_t;

typedef struct {
  uint32_t offset;
  uint32_t info;
  uint16_t addend;
} elf_relocate_addend_t, *elf_relocate_addend_ptr_t;

bool elf_check( elf_header_ptr_t );
bool elf_arch_check( elf_header_ptr_t );

#endif
