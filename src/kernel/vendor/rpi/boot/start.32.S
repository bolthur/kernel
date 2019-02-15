
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#define ASSEMBLER_FILE 1

.section .text.boot

.global boot_start
boot_start:
  // setup temporary stack
  ldr r4, =boot_start
  mov sp, r4

  // save boot parameter data
  push { r0 - r2 }

  // arch related boot start code
  bl boot_arch_start

  // clear bss section
  bl boot_bss_clear

  // prepare by vendor
  bl boot_vendor_prepare

  // restore boot parameter data
  pop { r0 - r2 }

  // load higher half start
  ldr r3, =start

  // set stack
  mov sp, r3

  // set program counter
  mov pc, r3
