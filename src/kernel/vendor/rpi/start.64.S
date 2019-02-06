
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
  #error "64 bit rpi startup not yet supported!"
  // setup temporary stack
  // FIXME: transform virtual to physical for higher half
  ldr x4, =start
  mov sp, x4

  // reset cpu when smp is enabled
  #if defined( SMP_ENABLED )
    bl cpu_smp_reset
  #endif

  // setup initial mmu for higher half
  #if defined( IS_HIGHER_HALF )
    #error "Higher half not yet supported!"
  #endif

  // clear bss section
  bl bss_clear

  // store rpi boot parameters passed with first three registers at data array
  ldr x4, =boot_parameter_data
  stmia x4!, {x0-x2}

  // continue with arch related start
  b arch_start
