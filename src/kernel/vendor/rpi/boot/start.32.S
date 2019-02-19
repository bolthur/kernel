
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

#define ASSEMBLER_FILE 1

#include "kernel/arch/arm/mm/virt.h"

// only when extension define is set
#if defined( BCM2709 ) || defined( BCM2710 )
  .arch_extension virt
#endif

.section .text.boot

.global boot_start
boot_start:
  // setup temporary stack
  ldr r3, =boot_start
  mov sp, r3

  // halt other cpus if smp enabled is set
  #if defined( SMP_ENABLED )
    // Return current CPU ID (0..3)
    cpsid if // Disable IRQ & FIQ
    mrc p15, 0, r3, c0, c0, 5 // r3 = Multiprocessor Affinity Register (MPIDR)
    ands r3, #3 // r0 = CPU ID (Bits 0..1)
    bne 1f // If (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)
    beq 2f // If equal branch to normal startup
    1:
      wfe
      b 1
    2:
  #endif

  #if defined( BCM2709 ) || defined( BCM2710 )
    // check for hypervisor mode and switch back to supervisor mode for rpi2 and rpi3
    mrs r3, cpsr
    eor r3, r3, #0x1A
    tst r3, #0x1F
    bic r3, r3, #0x1F // clear mode bits
    orr r3, r3, #( 0x13 | 0x40 | 0x80 ) // mask IRQ/FIQ bits and set SVC mode
    bne 3f // branch if not HYP mode
    orr r3, r3, #0x100 // mask Abort bit
    adr lr, 4f
    msr spsr_cxsf, r3
    msr ELR_hyp, lr
    eret
    3: msr cpsr_c, r3
    4:
      // setup temporary stack again
      ldr r4, =boot_start
      mov sp, r4
  #endif

  #if defined( BCM2709 )
    // Set SMP bit within auxiliary control register
    mrc p15, 0, r3, c1, c0, 1
    orr r3, r3, #( 1 << 6 )
    mcr p15, 0, r3, c1, c0, 1
  #elif defined( BCM2710 )
    // Set SMP bit within extended control register
    mrrc p15, 1, r3, r1, c15
    orr r3, r3, #( 1 << 6 )
    mcrr p15, 1, r3, r1, c15
  #endif

  // Disable caches within system controll register
  mrc p15, 0, r3, c1, c0, 0
  // disable data cache
  bic r3, r3, #( 1 << 2 )
  // disable instruction cache
  bic r3, r3, #( 1 << 12 )
  // push back changes to system control register
  mcr p15, 0, r3, c1, c0, 0

  // save boot parameter data
  push { r0 - r2 }

  // clear bss section
  bl boot_bss_clear

  // prepare by vendor
  bl boot_vendor_prepare

  // restore boot parameter data
  pop { r0 - r2 }

  // load higher half start
  ldr r3, =start

  // set stack
  mov sp, r3

  // set program counter
  mov pc, r3

// boot data section
.section .data.boot

// initial kernel context
.global initial_kernel_context
.balign SD_TTBR_ALIGNMENT_4G
initial_kernel_context: .fill SD_TTBR_SIZE_4G
