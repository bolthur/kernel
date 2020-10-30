
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#if ! defined( __ARCH_ARM_V7_CPU__ )
#define __ARCH_ARM_V7_CPU__

#define CPSR_MODE_USER 0x10
#define CPSR_MODE_FIQ 0x11
#define CPSR_MODE_IRQ 0x12
#define CPSR_MODE_SUPERVISOR 0x13
#define CPSR_MODE_MONITOR 0x16
#define CPSR_MODE_ABORT 0x17
#define CPSR_MODE_HYPERVISOR  0x1A
#define CPSR_MODE_UNDEFINED 0x1B
#define CPSR_MODE_SYSTEM 0x1F

#define CPSR_MODE_MASK 0x1F

#define CPSR_IRQ_INHIBIT 0x80
#define CPSR_FIQ_INHIBIT 0x40
#define CPSR_THUMB 0x20

#define STACK_FRAME_SIZE 68

#define SYS_CTRL_REG_ENABLE_DATA_CACHE 0x1 << 2
#define SYS_CTRL_REG_ENABLE_BRANCH_PREDICTION 0x1 << 11
#define SYS_CTRL_REG_ENABLE_INSTRUCTION_CACHE 0x1 << 12

#if ! defined( ASSEMBLER_FILE )
  #include <stdint.h>
  #include <core/debug/debug.h>

  /**
   * @brief CPU register context
   */
  typedef union __packed {
    uint32_t raw[ 17 ];
    struct {
      uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10; /* general purpose register */
      uint32_t fp; /* r11 = frame pointer */
      uint32_t ip; /* r12 = intraprocess scratch */
      uint32_t sp; /* r13 = stack pointer */
      uint32_t lr; /* r14 = link register */
      uint32_t pc; /* r15 = program counter */
      uint32_t spsr;
    } reg;
  } cpu_register_context_t, *cpu_register_context_ptr_t;

  /**
   * @brief Register map
   */
  typedef enum {
    R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC, CPSR
  } cpu_register_map_t, *cpu_register_map_ptr_t;

  #define DUMP_REGISTER( context ) \
    DEBUG_OUTPUT( \
      "CPU register dump\r\n"\
      "%4s: %#x\t%4s: %#x\t%4s: %#x\r\n"\
      "%4s: %#x\t%4s: %#x\t%4s: %#x\r\n"\
      "%4s: %#x\t%4s: %#x\t%4s: %#x\r\n"\
      "%4s: %#x\t%4s: %#x\t%4s: %#x\r\n"\
      "%4s: %#x\t%4s: %#x\t%4s: %#x\r\n"\
      "%4s: %#x\t%4s: %#x\r\n", \
      "r0", ( ( cpu_register_context_ptr_t )context )->reg.r0, \
      "r1", ( ( cpu_register_context_ptr_t )context )->reg.r1, \
      "r2", ( ( cpu_register_context_ptr_t )context )->reg.r2, \
      "r3", ( ( cpu_register_context_ptr_t )context )->reg.r3, \
      "r4", ( ( cpu_register_context_ptr_t )context )->reg.r4, \
      "r5", ( ( cpu_register_context_ptr_t )context )->reg.r5, \
      "r6", ( ( cpu_register_context_ptr_t )context )->reg.r6, \
      "r7", ( ( cpu_register_context_ptr_t )context )->reg.r7, \
      "r8", ( ( cpu_register_context_ptr_t )context )->reg.r8, \
      "r9", ( ( cpu_register_context_ptr_t )context )->reg.r9, \
      "r10", ( ( cpu_register_context_ptr_t )context )->reg.r10, \
      "fp", ( ( cpu_register_context_ptr_t )context )->reg.fp, \
      "ip", ( ( cpu_register_context_ptr_t )context )->reg.ip, \
      "sp", ( ( cpu_register_context_ptr_t )context )->reg.sp, \
      "lr", ( ( cpu_register_context_ptr_t )context )->reg.lr, \
      "pc", ( ( cpu_register_context_ptr_t )context )->reg.pc, \
      "spsr", ( ( cpu_register_context_ptr_t )context )->reg.spsr \
    )
#endif

#endif
