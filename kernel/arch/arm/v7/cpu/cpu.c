
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

#include <stdio.h>

#include "arch/arm/v7/cpu.h"

/**
 * @brief Method for dumping cpu registers
 *
 * @param context cpu context with all registers
 *
 * @todo check and revise
 */
void dump_register( cpu_register_context_t *context ) {
  printf(
    "CPU register dump\r\n"\
    "r0: 0x%08x\tr1: 0x%08x\tr2: 0x%08x\r\n"\
    "r3: 0x%08x\tr4: 0x%08x\tr5: 0x%08x\r\n"\
    "r6: 0x%08x\tr7: 0x%08x\tr8: 0x%08x\r\n"\
    "r9: 0x%08x\tr10: 0x%08x\tfp: 0x%08x\r\n"\
    "ip: 0x%08x\tsp: 0x%08x\tlr: 0x%08x\r\n"\
    "pc: 0x%08x\tspsr: 0x08%x\r\n",
    context->reg.r0, context->reg.r1, context->reg.r2,
    context->reg.r3, context->reg.r4, context->reg.r5,
    context->reg.r6, context->reg.r7, context->reg.r8,
    context->reg.r9, context->reg.r10, context->reg.fp,
    context->reg.ip, context->reg.sp, context->reg.lr,
    context->reg.pc, context->reg.spsr
  );
}
