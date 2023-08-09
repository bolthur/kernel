/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include "../../../assembly.h"

.section .text.boot

/**
 * @brief Method to enable floating point unit when fpu enabled flag is set
 */
EXPORT( fpu_enable )
fpu_enable:
  // read coprocessor access control register into r0
  mrc p15, 0, r0, c1, c0, 2
  // enable full access for everyone
  orr r0, #( 0xf << 20 )
  // write back changes
  mcr p15, 0, r0, c1, c0, 2

  // read again coprocessor access control register into r0 to check for fpu
  mrc p15, 0, r0, c1, c0, 2
  // get 4 bits starting at bit 20
  ands r0, #( 0xf << 20 )
  // skip enable if not supported
  bne 1f // If not 0 enable fpu ( detected )
  beq 2f // If 0 skip enable
  1:
    // Set FPEXC.EN bit to enable fpu
    mov r0, #( 1 << 30 )
    // Transfer change to vfp system register
    vmsr fpexc, r0
  2:
    bx lr
