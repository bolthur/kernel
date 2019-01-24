
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

/**
 * @brief Method to enable floating point unit when fpu enabled flag is set
 */
.global fpu_enable
fpu_enable:
  #if defined( FPU_ENABLED )
    // get content from NSACR for enable of fpu for secure and non secure mode
    mrc p15, 0, r0, c1, c1, 2

    // set bit 10 and 11 for access to cp10 and cp11 within secure and non secure
    orr r0, r0, #( 0x3 << 10 )

    // write back changes to NSACR
    mcr p15, 0, r0, c1, c1, 2

    // read co processor access controll register into r0
    mrc p15, 0, r0, c1, c0, 2

    // full access for everyone
    orr r0, r0, #( 0xf << 20 )

    // write back changes
    mcr p15, 0, r0, c1, c0, 2

    // Set FPEXC.EN bit to enable fpu
    mov r0, #( 0x1 << 30 )

    // Transfer change to vfp system register
    vmsr fpexc, r0
  #endif
  bx lr
