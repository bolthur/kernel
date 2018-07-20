
/**
 * mist-system/kernel
 * Copyright (C) 2017 mist-system project.
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

#define ASM_FILE 1
#include <vendor/pc/multiboot2.h>

.section .multiboot
.code32
// 64 bit boundary
.align 8
multiboot_header:
  // magic
  .long MULTIBOOT2_HEADER_MAGIC
  // ISA: i386
  .long MULTIBOOT_ARCHITECTURE_I386
  // Header length.
  .long multiboot_header_end - multiboot_header
  // checksum
  .long -( MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + ( multiboot_header_end - multiboot_header ) )
framebuffer_tag_start:
  .short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
  .short MULTIBOOT_HEADER_TAG_OPTIONAL
  .long framebuffer_tag_end - framebuffer_tag_start
  .long 1024
  .long 768
  .long 32
framebuffer_tag_end:
  .short MULTIBOOT_HEADER_TAG_END
  .short 0
  .long 0
multiboot_header_end:

.section .text

.global start
.type start, @function
start:
  // initialize stack
  movl $stack, %esp

  // reset EFLAGS.
  pushl $0
  popf

  // store grub parameter at data array
  mov $(boot_parameter_data), %ebx // multiboot structure
  mov $(boot_parameter_data+4), %eax // multiboot magic value

  // jump to arch start
  jmp arch_start

// data area
.section .bss

// stack
.space 8192
stack:
