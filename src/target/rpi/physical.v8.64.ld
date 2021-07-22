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

OUTPUT_FORMAT( elf64-littleaarch64 )
OUTPUT_ARCH( arm )

ENTRY( startup )

KERNEL_LMA = 0x80000;
KERNEL_OFFSET = 0xffffffff80000000;

SECTIONS {
  . = KERNEL_LMA;
  __kernel_start = . + KERNEL_OFFSET;

  /* boot kernel sections */
  .text.boot : ALIGN( 4K ) {
    *( .text.boot )
  }

  .data.boot : ALIGN( 4K ) {
    *( .data.boot )
  }

  . += KERNEL_OFFSET;

  /* higher half kernel sections */
  .text ALIGN( 4K ) : AT( ADDR( .text ) - KERNEL_OFFSET ) {
    *( .text )
  }

  .rodata ALIGN( 4K ) : AT( ADDR( .rodata ) - KERNEL_OFFSET ) {
    *( .rodata )
    *( .data )
  }

  .bss ALIGN( 4K ) : AT( ADDR( .bss ) - KERNEL_OFFSET ) {
    __bss_start = .;
    *( COMMON )
    *( .bss )

    /* align to 4k */
    . = ALIGN( 4K );
    /* 1MB for initial heap */
    __initial_heap_start = .;
    . += 0x100000;
    __initial_heap_end = .;

    __bss_end = .;
  }

  .fdt ALIGN( 4K ) : AT( ADDR( .fdt ) - KERNEL_OFFSET ) {
    __fdt_start = .;
    *( .fdt )
    __fdt_end = .;
  }

  __kernel_end = .;
}
