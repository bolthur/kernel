
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

// forward declaration for save of parameter
.extern boot_parameter_data

.section .text.boot

.global start
start:
  // reset multicore environment
  bl arch_cpu_reset

  #if defined(ELF32)
    // setup temporary stack that will be replaced later
    ldr r4, =start
    mov sp, r4

    // clear out bss
    ldr r4, =__bss_start
    ldr r9, =__bss_end
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
  #elif defined(ELF64)
    #error "ELF64 not yet supported!"
  #else
    #error "No valid format set. Valid formats are ELF32 and ELF64"
  #endif

  b .test

.loop:
  // this does 4x4 = 16 byte stores at once
  #if defined(ELF32)
    stmia r4!, {r5-r8}  // the '!' increments r4 but only after ('ia') the store
  #elif defined(ELF64)
    #error "ELF64 not yet supported!"
  #else
    #error "No valid format set. Valid formats are ELF32 and ELF64"
  #endif

.test:
  #if defined(ELF32)
    cmp r4, r9
  #elif defined(ELF64)
    #error "ELF64 not yet supported!"
  #else
    #error "No valid format set. Valid formats are ELF32 and ELF64"
  #endif
  blo .loop

  // store boot parameters from r0 - r2 /  at data array ( only set on real device )
  #if defined(ELF32)
    ldr r4, =boot_parameter_data
    stmia r4!, {r0-r2}
  #elif defined(ELF64)
    #error "ELF64 not yet supported!"
  #else
    #error "No valid format set. Valid formats are ELF32 and ELF64"
  #endif

  // continue with arch related start
  b arch_start
