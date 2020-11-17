
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

#if ! defined( __CORE_ELF_COMMON__ )
#define __CORE_ELF_COMMON__

#include <stdbool.h>
#include <stdint.h>
#include <core/task/process.h>

// e_ident
#define ELF_NIDENT 16
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFDATA2LSB 1 // Little Endian
#define ELFDATA2MSB 1 // Big Endian
#define ELFCLASS32 1 // 32-bit Architecture
#define ELFCLASS64 1 // 64-bit Architecture

enum Elf_Ident {
  EI_MAG0 = 0,
  EI_MAG1 = 1,
  EI_MAG2 = 2,
  EI_MAG3 = 3,
  EI_CLASS = 4,
  EI_DATA = 5,
  EI_VERSION = 6,
  EI_OSABI = 7,
  EI_ABIVERSION = 8,
  EI_PAD = 9,
};

// e_type
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

// e_machine
#define EM_NONE 0
#define EM_ARM 0x28
#define EM_AARCH64 0xb7

// e_version
#define EV_NONE 0
#define EV_CURRENT 1

// p_type
#define PT_NULL 0x0
#define PT_LOAD 0x1
#define PT_DYNAMIC 0x2
#define PT_INTERP 0x3
#define PT_NOTE 0x4
#define PT_SHLIB 0x5
#define PT_PHDR 0x6

// p_flags
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

// sh_type
#define SHT_NULL 0x0
#define SHT_PROGBITS 0x1
#define SHT_SYMTAB 0x2
#define SHT_STRTAB 0x3
#define SHT_RELA 0x4
#define SHT_HASH 0x5
#define SHT_DYNAMIC 0x6
#define SHT_NOTE 0x7
#define SHT_NOBITS 0x8
#define SHT_REL 0x9

// sh_flags
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4

// st_info
#define ELF32_ST_BIND( info ) ( info >> 4 )
#define ELF32_ST_TYPE( info ) ( info & 0xf )
#define ELF32_ST_INFO( bind, type ) ( ( bind << 4 ) + ( type & 0xf ) )

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_COMMON 5

// st_other
#define ELF32_ST_VISIBILITY( o ) ( o & 0x3 )

// r_info
#define ELF32_R_SYM( info ) ( info >> 8 )
#define ELF32_R_TYPE( info ) ( ( uint8_t )info )
#define ELF32_R_INFO( sym, type ) ( ( bind << 8 ) + ( ( uint8_t )type ) )

bool elf_check( uintptr_t );
bool elf_arch_check( uintptr_t );
uintptr_t elf_load( uintptr_t, task_process_ptr_t );

#endif
