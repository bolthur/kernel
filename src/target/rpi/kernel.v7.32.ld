
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

OUTPUT_FORMAT( elf32-littlearm )
OUTPUT_ARCH( arm )

ENTRY( boot_start )

GROUP(
  ../../arch/arm/v7/libv7.a
  ../../arch/arm/libarm.a
  ../../core/libcore.a
  ../../platform/rpi/librpi.a
  ../../lib/assert/libassert.a
  ../../lib/atag/libatag.a
  ../../lib/avl/libavl.a
  ../../lib/list/liblist.a
  ../../lib/ssp/libssp.a
  ../../lib/stdio/libstdio.a
  ../../lib/stdlib/libstdlib.a
  ../../lib/string/libstring.a
  ../../lib/tar/libtar.a
  ../../lib/ubsan/libubsan.a
)
INPUT(
  libgcc.a
)

KERNEL_LMA = 0x8000;
KERNEL_OFFSET = 0xC0000000;

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

  . += 0x20000;
  /* dummy user task */
  .text.dummytask : ALIGN( 4K ) {
    *( .text.dummytask )
  }

  . += KERNEL_OFFSET;

  /* higher half kernel sections */
  .text ALIGN( 4K ) : AT( ADDR( .text ) - KERNEL_OFFSET ) {
    *( .text )
    *( .rodata )
  }

  .data ALIGN( 4K ) : AT( ADDR( .data ) - KERNEL_OFFSET ) {
    *( .data )
  }

  .bss ALIGN( 4K ) : AT( ADDR( .bss ) - KERNEL_OFFSET ) {
    __bss_start = .;
    *( COMMON )
    *( .bss )
    __bss_end = .;
  }

  __kernel_end = .;

  /DISCARD/ : {
    *( .comment )
    *( .note )
    *( .eh_frame )
    *( .ARM.exidx* )
    *( .ARM.attributes* )
  }
}
