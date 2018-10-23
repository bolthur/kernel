
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

OUTPUT_FORMAT( elf32-littlearm )
OUTPUT_ARCH( arm )

ENTRY( start )

GROUP(
  ../../_arch/_arm/_v6/libv6.a
  ../../_arch/_arm/libarm.a
  ../../libkernel.a
  ../libvendor.a
  librpi.a
  ../../../lib/avl/libavl.a
  ../../../lib/stdc/libstdc.a
  ../../../lib/tar/libtar.a
  libfdt.a
)
INPUT(
  libgcc.a
)

SECTIONS {
  . = 0x10000;
  __kernel_start = .;

  .text BLOCK( 4K ): ALIGN( 4K ) {
    KEEP( *( .text.boot ) )
    *( .text )
  }

  .rodata BLOCK( 4K ): ALIGN( 4K ) {
    *(.rodata)
  }

  .data BLOCK( 4K ): ALIGN( 4K ) {
    *(.data)
  }

  .bss BLOCK( 4K ): ALIGN( 4K ) {
    __bss_start = .;
    *(COMMON)
    *(.bss)
    __bss_end = .;
  }

  .bugtable BLOCK( 4K ): ALIGN( 4K ) {
    __bug_start = . ;
    *(.bugtable)
    __bug_end = . ;
  }

  .ARM.exidx BLOCK( 4K ): ALIGN( 4K ) {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  }
  __kernel_end = .;
}
