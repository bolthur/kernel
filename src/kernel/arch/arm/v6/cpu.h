
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

#ifndef __KERNEL_ARCH_ARM_V6_CPU__
#define __KERNEL_ARCH_ARM_V6_CPU__

#include <stdint.h>

typedef struct {
  uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10; /* general purpose register */
  uint32_t fp; /* r11 = frame pointer */
  uint32_t ip; /* r12 = intraprocess scratch */
  uint32_t sp; /* r13 = stack pointer */
  uint32_t lr; /* r14 = link register */
  uint32_t pc; /* r15 = program counter */
} cpu_register_t;

#endif
