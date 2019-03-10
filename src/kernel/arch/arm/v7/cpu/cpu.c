
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

#include "lib/k/stdio.h"
#include "kernel/arch/arm/v7/cpu.h"

/**
 * @brief Method for dumping cpu registers
 *
 * @param context cpu context with all registers
 *
 * @todo check and revise function
 */
void dump_register( cpu_register_context_t* context ) {
  printf(
    "CPU register dump\r\n"\
    "%4s: 0x%08x\t%4s: 0x%08x\t%4s: 0x%08x\r\n"\
    "%4s: 0x%08x\t%4s: 0x%08x\t%4s: 0x%08x\r\n"\
    "%4s: 0x%08x\t%4s: 0x%08x\t%4s: 0x%08x\r\n"\
    "%4s: 0x%08x\t%4s: 0x%08x\t%4s: 0x%08x\r\n"\
    "%4s: 0x%08x\t%4s: 0x%08x\t%4s: 0x%08x\r\n"\
    "%4s: 0x%08x\t%4s: 0x%08x\r\n",
    "r0", context->reg.r0, "r1", context->reg.r1,
    "r2", context->reg.r2, "r3", context->reg.r3,
    "r4", context->reg.r4, "r5", context->reg.r5,
    "r6", context->reg.r6, "r7", context->reg.r7,
    "r8", context->reg.r8, "r9", context->reg.r9,
    "r10", context->reg.r10, "fp", context->reg.fp,
    "ip", context->reg.ip, "sp", context->reg.sp,
    "lr", context->reg.lr, "pc", context->reg.pc,
    "spsr", context->reg.spsr
  );
}
