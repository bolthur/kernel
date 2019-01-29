
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

// forward declaration for save of parameter
.extern boot_parameter_data

.section .text.boot

.global start
start:
  // reset cpu when smp is enabled
  #if defined( SMP_ENABLED )
    bl cpu_smp_reset
  #endif

  // setup initial mmu for higher half
  #if defined( IS_HIGHER_HALF )
    // #error "Higher half not yet supported!"
  #endif

  #if defined( ELF32 )
    // FIXME: transform virtual to physical for higher half
    // setup temporary stack, which is going to be replaced later
    ldr r4, =start
    mov sp, r4
  #elif defined( ELF64 )
    #error "ELF64 not yet supported!"
  #else
    #error "No valid format set. Valid formats are ELF32 and ELF64"
  #endif

  // clear bss section
  bl bss_clear

  // store boot parameters from r0 - r2 /  at data array ( only set on real device )
  #if defined( ELF32 )
    ldr r4, =boot_parameter_data
    stmia r4!, {r0-r2}
  #elif defined( ELF64 )
    #error "ELF64 not yet supported!"
  #else
    #error "No valid format set. Valid formats are ELF32 and ELF64"
  #endif

  // continue with arch related start
  b arch_start
