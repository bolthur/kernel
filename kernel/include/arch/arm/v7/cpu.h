
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

#ifndef __KERNEL_ARCH_ARM_V7_CPU__
#define __KERNEL_ARCH_ARM_V7_CPU__

#define CPSR_MODE_USER 0x10
#define CPSR_MODE_FIQ 0x11
#define CPSR_MODE_IRQ 0x12
#define CPSR_MODE_SUPERVISOR 0x13
#define CPSR_MODE_MONITOR 0x16
#define CPSR_MODE_ABORT 0x17
#define CPSR_MODE_HYPERVISOR  0x1A
#define CPSR_MODE_UNDEFINED 0x1B
#define CPSR_MODE_SYSTEM 0x1F

#define CPSR_IRQ_INHIBIT 0x80
#define CPSR_FIQ_INHIBIT 0x40
#define CPSR_THUMB 0x20

#define STACK_FRAME_SIZE 68

#define SYS_CTRL_REG_ENABLE_DATA_CACHE 0x1 << 2
#define SYS_CTRL_REG_ENABLE_BRANCH_PREDICTION 0x1 << 11
#define SYS_CTRL_REG_ENABLE_INSTRUCTION_CACHE 0x1 << 12

#ifndef ASSEMBLER_FILE
#include <stdint.h>

#if defined( __cplusplus )
extern "C" {
#endif

typedef union {
  uint32_t storage[ 17 ];
  struct {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10; /* general purpose register */
    uint32_t fp; /* r11 = frame pointer */
    uint32_t ip; /* r12 = intraprocess scratch */
    uint32_t sp; /* r13 = stack pointer */
    uint32_t lr; /* r14 = link register */
    uint32_t pc; /* r15 = program counter */
    uint32_t spsr;
  } reg;
} __attribute__((__packed__)) cpu_register_context_t;

void cpu_invalidate_cache( void );
void cpu_disable_cache( void );
void cpu_enable_cache( void );

void dump_register( cpu_register_context_t *context );

#if defined( __cplusplus )
}
#endif

#endif

#endif
