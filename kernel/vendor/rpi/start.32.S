
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

#include "kernel/entry.h"

// forward declaration for save of parameter
.extern loader_parameter_data

.section .text.boot

.global start
start:
  // setup temporary stack
  ldr r4, =start - KERNEL_OFFSET
  mov sp, r4

  // reset cpu when smp is enabled
  #if defined( SMP_ENABLED )
    // get address of smp reset code
    ldr r4, =cpu_smp_reset - KERNEL_OFFSET
    // execute function
    blx r4
  #endif

  // clear bss section
  ldr r4, =bss_clear - KERNEL_OFFSET
  // execute function
  blx r4

  // hyp mode fix
  // ldr r4, =hyp_mode_fix - KERNEL_OFFSET
  // execute function
  // blx r4

  // setup initial mmu for higher half
  #if defined( IS_HIGHER_HALF )
    // get address of function
    // ldr r4, =virt_setup_initial - KERNEL_OFFSET
    // execute function
    // blx r4

    // reset stack again
    // ldr r4, =start
    // mov sp, r4
  #endif

  // store rpi boot parameters passed with first three registers at data array
  ldr r4, =loader_parameter_data
  stmia r4!, {r0-r2}

  // continue with arch related start
  b arch_start
