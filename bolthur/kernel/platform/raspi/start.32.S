/**
 * Copyright (C) 2018 - 2026 bolthur project.
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
#include "../../arch/arm/v7/cpu.h"
#include "../../assembly.h"
#include "../../entry.h"

// forward declaration for save of parameter
IMPORT( firmware_info )
// import used functions
IMPORT( bss_startup_clear )
IMPORT( fpu_enable )
IMPORT( virt_startup_setup )
IMPORT( firmware_startup_init )
IMPORT( initrd_startup_init )

// only when extension define is set
#if defined( BCM2709 ) || defined( BCM2710 )
  .arch_extension virt
  .arch_extension sec
#endif

.section .text.boot

EXPORT( startup )
startup:
  b reset

reset:
  // setup temporary stack
  ldr r3, =startup
  mov sp, r3

  // halt other cpus if smp enabled is set
  #if defined( BCM2709 ) || defined( BCM2710 )
    // Return current CPU ID (0..3)
    cpsid iaf // Disable IRQ & FIQ
    mrc p15, 0, r3, c0, c0, 5 // r3 = Multiprocessor Affinity Register (MPIDR)
    ands r3, #3 // r0 = CPU ID (Bits 0..1)
    bne 1f // If (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)
    beq 2f // If equal branch to normal startup
    1:
      wfe
      b 1
    2:
  #endif

  // switch to svc mode if necessary
  #if defined( BCM2709 ) || defined( BCM2710 )
    // check for hypervisor mode and switch back to supervisor mode for raspi2b and raspi3b
    mrs r3, cpsr
    eor r3, #CPSR_MODE_HYPERVISOR
    tst r3, #CPSR_MODE_MASK
    bic r3, #CPSR_MODE_MASK // clear mode bits
    orr r3, #( CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT ) // mask IRQ/FIQ bits and set SVC mode
    bne 3f // branch if not HYP mode
    orr r3, #0x100 // mask Abort bit
    adr lr, 4f
    // return from hyp mode
    msr spsr_cxsf, r3
    msr elr_hyp, lr
    // reset hypervisor configuration register
    mov r4, #0
    mcr p15, 4, r4, c1, c1, 0
    // modify cnthctl
    mrc p15, 4, r4, c14, c1, 0
    orr r4, r4, #3
    mcr p15, 4, r4, c14, c1, 0
    // barriers
    dmb
    isb
    // return from hyp mode
    eret
    3: msr cpsr_c, r3
    4:
      isb
      // setup temporary stack again
      ldr r4, =startup
      mov sp, r4
      isb
  #endif

  // smp bit
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

  // Disable caches within system control register
  mrc p15, 0, r3, c1, c0, 0
  // disable data cache
  bic r3, r3, #( 1 << 2 )
  // disable instruction cache
  bic r3, r3, #( 1 << 12 )
  // push back changes to system control register
  mcr p15, 0, r3, c1, c0, 0

  // save boot parameter data
  push { r0 - r2 }
  // enable fpu
  bl fpu_enable
  // clear bss section
  bl bss_startup_clear
  // restore boot parameter data
  pop { r0 - r2 }

  // store raspi boot parameters passed with first three registers at data array
  ldr r3, =firmware_info - KERNEL_OFFSET
  stmia r3!, { r0 - r2 }

  // initial virtual memory setup
  bl virt_startup_setup
  // ongoing calls are higher half located
  // startup related init
  bl firmware_startup_init
  // startup related initrd init
  bl initrd_startup_init

  // load higher half start
  ldr r3, =start
  // set stack
  mov sp, r3
  // set program counter
  mov pc, r3

.section .text

EXPORT( start )
start:
  // continue with arch related start
  b arch_start
