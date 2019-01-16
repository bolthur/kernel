
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
 * @brief Method to enable floating point unit
 *
 * @todo check and revise
 */
.global fpu_enable
fpu_enable:
  mrc p15, 0, r0, c1, c0, 2
  orr r0, r0, #0x300000 // single precision
  orr r0, r0, #0xC00000 // double precision
  mcr p15, 0, r0, c1, c0, 2
  mov r0, #0x40000000
  fmxr fpexc, r0
  bx lr