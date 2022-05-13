/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#if ! defined( _ARCH_ARM_V7_CPU_H )
#define _ARCH_ARM_V7_CPU_H

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

#define CPSR_THUMB 1 << 5
#define CPSR_FIQ_INHIBIT 1 << 6
#define CPSR_IRQ_INHIBIT 1 << 7

#define PC_OFFSET 60
#define SPSR_OFFSET 64
#define SP_OFFSET 52
#define LR_OFFSET 56
#if defined( ARM_CPU_HAS_NEON )
  #define STACK_FRAME_SIZE 328
  #define GENERAL_PURPOSE_REGISTER_END 68
  #define NEON_REGISTER_START 72
  #define PRIVILEGED_PC_OFFSET 328
  #define PRIVILEGED_SPSR_OFFSET 332
#else
  #define STACK_FRAME_SIZE 68
  #define GENERAL_PURPOSE_REGISTER_END 68
  #define PRIVILEGED_PC_OFFSET 68
  #define PRIVILEGED_SPSR_OFFSET 72
#endif

#define SYS_CTRL_REG_ENABLE_DATA_CACHE 1 << 2
#define SYS_CTRL_REG_ENABLE_BRANCH_PREDICTION 1 << 11
#define SYS_CTRL_REG_ENABLE_INSTRUCTION_CACHE 1 << 12

#if ! defined( ASSEMBLER_FILE )
  #include <stdint.h>
  #include "../../../debug/debug.h"

  /**
   * @brief CPU register context
   */
  typedef union __packed {
    #if defined( ARM_CPU_HAS_NEON )
      uint32_t raw[ 82 ];
    #else
      uint32_t raw[ 17 ];
    #endif
    struct {
      /* general purpose register */
      uint32_t r0;
      uint32_t r1;
      uint32_t r2;
      uint32_t r3;
      uint32_t r4;
      uint32_t r5;
      uint32_t r6;
      uint32_t r7;
      uint32_t r8;
      uint32_t r9;
      uint32_t r10;
      uint32_t fp; /* r11 = frame pointer */
      uint32_t ip; /* r12 = intraprocess scratch */
      uint32_t sp; /* r13 = stack pointer */
      uint32_t lr; /* r14 = link register */
      uint32_t pc; /* r15 = program counter */
      uint32_t spsr;
      #if defined( ARM_CPU_HAS_NEON )
        uint32_t fpscr;
        uint64_t neon[ 32 ];
      #endif
    } reg;
  } cpu_register_context_t;

  /**
   * @brief Register map
   */
  typedef enum {
    R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC, CPSR
  } cpu_register_map_t;

  #if defined( ARM_CPU_HAS_NEON )
    #define DUMP_REGISTER( context ) \
      DEBUG_OUTPUT( "CPU register dump:\r\n" ) \
      DEBUG_OUTPUT( "  r0: %#08x, r1: %#08x,  r2: %#08x, r3: %#08x\r\n", ( ( cpu_register_context_t* )context )->reg.r0, ( ( cpu_register_context_t* )context )->reg.r1,  ( ( cpu_register_context_t* )context )->reg.r2, ( ( cpu_register_context_t* )context )->reg.r3 ) \
      DEBUG_OUTPUT( "  r4: %#08x, r5: %#08x,  r6: %#08x, r7: %#08x\r\n", ( ( cpu_register_context_t* )context )->reg.r4, ( ( cpu_register_context_t* )context )->reg.r5,  ( ( cpu_register_context_t* )context )->reg.r6, ( ( cpu_register_context_t* )context )->reg.r7 ) \
      DEBUG_OUTPUT( "  r8: %#08x, r9: %#08x, r10: %#08x, fp: %#08x\r\n", ( ( cpu_register_context_t* )context )->reg.r8, ( ( cpu_register_context_t* )context )->reg.r9, ( ( cpu_register_context_t* )context )->reg.r10, ( ( cpu_register_context_t* )context )->reg.fp ) \
      DEBUG_OUTPUT( "  ip: %#08x, sp: %#08x,  lr: %#08x, pc: %#08x\r\n", ( ( cpu_register_context_t* )context )->reg.ip, ( ( cpu_register_context_t* )context )->reg.sp,  ( ( cpu_register_context_t* )context )->reg.lr, ( ( cpu_register_context_t* )context )->reg.pc ) \
      DEBUG_OUTPUT( "spsr: %#08x\r\n", ( ( cpu_register_context_t* )context )->reg.spsr ) \
      DEBUG_OUTPUT( "floating-point status and control register: %#08x\r\n", ( ( cpu_register_context_t* )context )->reg.fpscr )  \
      DEBUG_OUTPUT( "floating-point register dump:\r\n" ) \
      DEBUG_OUTPUT( " d0: %#016llx,  d1: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 0 ], ( ( cpu_register_context_t* )context )->reg.neon[ 1 ] ) \
      DEBUG_OUTPUT( " d2: %#016llx,  d3: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 2 ], ( ( cpu_register_context_t* )context )->reg.neon[ 3 ] ) \
      DEBUG_OUTPUT( " d4: %#016llx,  d5: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 4 ], ( ( cpu_register_context_t* )context )->reg.neon[ 5 ] ) \
      DEBUG_OUTPUT( " d6: %#016llx,  d7: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 6 ], ( ( cpu_register_context_t* )context )->reg.neon[ 7 ] ) \
      DEBUG_OUTPUT( " d8: %#016llx,  d9: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 8 ], ( ( cpu_register_context_t* )context )->reg.neon[ 9 ] ) \
      DEBUG_OUTPUT( "d10: %#016llx, d11: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 10 ], ( ( cpu_register_context_t* )context )->reg.neon[ 11 ] ) \
      DEBUG_OUTPUT( "d12: %#016llx, d13: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 12 ], ( ( cpu_register_context_t* )context )->reg.neon[ 13 ] ) \
      DEBUG_OUTPUT( "d14: %#016llx, d15: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 14 ], ( ( cpu_register_context_t* )context )->reg.neon[ 15 ] ) \
      DEBUG_OUTPUT( "d16: %#016llx, d17: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 16 ], ( ( cpu_register_context_t* )context )->reg.neon[ 17 ] ) \
      DEBUG_OUTPUT( "d18: %#016llx, d19: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 18 ], ( ( cpu_register_context_t* )context )->reg.neon[ 19 ] ) \
      DEBUG_OUTPUT( "d20: %#016llx, d21: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 20 ], ( ( cpu_register_context_t* )context )->reg.neon[ 21 ] ) \
      DEBUG_OUTPUT( "d22: %#016llx, d23: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 22 ], ( ( cpu_register_context_t* )context )->reg.neon[ 23 ] ) \
      DEBUG_OUTPUT( "d24: %#016llx, d25: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 24 ], ( ( cpu_register_context_t* )context )->reg.neon[ 25 ] ) \
      DEBUG_OUTPUT( "d26: %#016llx, d27: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 26 ], ( ( cpu_register_context_t* )context )->reg.neon[ 27 ] ) \
      DEBUG_OUTPUT( "d28: %#016llx, d29: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 28 ], ( ( cpu_register_context_t* )context )->reg.neon[ 29 ] ) \
      DEBUG_OUTPUT( "d30: %#016llx, d31: %#016llx\r\n", ( ( cpu_register_context_t* )context )->reg.neon[ 30 ], ( ( cpu_register_context_t* )context )->reg.neon[ 31 ] )
  #else
    #define DUMP_REGISTER( context ) \
      DEBUG_OUTPUT( \
        "CPU register dump\r\n"\
        "%4s: %#08x\t%4s: %#08x\t%4s: %#08x\r\n"\
        "%4s: %#08x\t%4s: %#08x\t%4s: %#08x\r\n"\
        "%4s: %#08x\t%4s: %#08x\t%4s: %#08x\r\n"\
        "%4s: %#08x\t%4s: %#08x\t%4s: %#08x\r\n"\
        "%4s: %#08x\t%4s: %#08x\t%4s: %#08x\r\n"\
        "%4s: %#08x\t%4s: %#08x\r\n", \
        "r0", ( ( cpu_register_context_t* )context )->reg.r0, \
        "r1", ( ( cpu_register_context_t* )context )->reg.r1, \
        "r2", ( ( cpu_register_context_t* )context )->reg.r2, \
        "r3", ( ( cpu_register_context_t* )context )->reg.r3, \
        "r4", ( ( cpu_register_context_t* )context )->reg.r4, \
        "r5", ( ( cpu_register_context_t* )context )->reg.r5, \
        "r6", ( ( cpu_register_context_t* )context )->reg.r6, \
        "r7", ( ( cpu_register_context_t* )context )->reg.r7, \
        "r8", ( ( cpu_register_context_t* )context )->reg.r8, \
        "r9", ( ( cpu_register_context_t* )context )->reg.r9, \
        "r10", ( ( cpu_register_context_t* )context )->reg.r10, \
        "fp", ( ( cpu_register_context_t* )context )->reg.fp, \
        "ip", ( ( cpu_register_context_t* )context )->reg.ip, \
        "sp", ( ( cpu_register_context_t* )context )->reg.sp, \
        "lr", ( ( cpu_register_context_t* )context )->reg.lr, \
        "pc", ( ( cpu_register_context_t* )context )->reg.pc, \
        "spsr", ( ( cpu_register_context_t* )context )->reg.spsr \
      )
  #endif
#endif

#endif
