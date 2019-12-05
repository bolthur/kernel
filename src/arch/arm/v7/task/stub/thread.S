
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

#define ASSEMBLER_FILE 1
#include <kernel/assembly.h>
#include <arch/arm/v7/cpu.h>

EXPORT( switch_to_thread )
switch_to_thread:
  // set fp
  mov fp, r0
  // load spsr
  ldr r0, [ fp, #64 ]
  // load lr
  ldr lr, [ fp, #60 ]
  // set spsr
  msr spsr_cxsf, r0
  // restore registers
  ldmia fp, { r0 - r14 }^
  nop
  // return to process
  movs pc, lr
