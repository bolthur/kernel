
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

#define ASSEMBLER_FILE 1
#include <arch/arm/v7/cpu.h>
#include <core/assembly.h>
#include <core/entry.h>

// forward declaration for save of parameter
IMPORT( loader_parameter_data )

// only when extension define is set
#if defined( BCM2709 ) || defined( BCM2710 )
  .arch_extension virt
  .arch_extension sec
#endif

.section .text.boot

EXPORT( boot_start )
boot_start:
  b reset
  #if defined( BCM2709 ) || defined( BCM2710 )
    b hang
    b smc
    b hang
    b hang
    b hang
    b hang
    b hang
  #endif

#if defined( BCM2709 ) || defined( BCM2710 )
  hang:
    b hang

  smc:
    // reset scr
    mov r1, #0
    mcr p15, 0, r1, c1, c1, 0
    isb
    movs pc, lr
#endif

reset:
  // setup temporary stack
  ldr r3, =boot_start
  mov sp, r3

  // halt other cpus if smp enabled is set
  #if defined( BCM2709 ) || defined( BCM2710 )
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

  // switch to svc mode if necessary
  #if defined( BCM2709 ) || defined( BCM2710 )
    // check for hypervisor mode and switch back to supervisor mode for rpi2 and rpi3
    mrs r3, cpsr
    eor r3, #CPSR_MODE_HYPERVISOR
    tst r3, #CPSR_MODE_MASK
    bic r3, #CPSR_MODE_MASK // clear mode bits
    orr r3, #( CPSR_MODE_SUPERVISOR | CPSR_FIQ_INHIBIT | CPSR_IRQ_INHIBIT ) // mask IRQ/FIQ bits and set SVC mode
    bne 3f // branch if not HYP mode
    orr r3, #0x100 // mask Abort bit
    adr lr, 4f
    msr spsr_cxsf, r3
    msr elr_hyp, lr
    eret
    3: msr cpsr_c, r3
    4:
      // setup temporary stack again
      ldr r4, =boot_start
      mov sp, r4
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
  // restore boot parameter data
  pop { r0 - r2 }

  // store rpi boot parameters passed with first three registers at data array
  ldr r3, =loader_parameter_data - KERNEL_OFFSET
  stmia r3!, { r0 - r2 }

  #if defined( BCM2709 ) || defined( BCM2710 )
    #if defined( QEMU_BUILD )
      // qemu hack to overwrite mvbar which starts at 0x400
      mov r1, #0x400
      ldr r0, =boot_start
      ldmia r0!,{ r2 - r9 }
      stmia r1!,{ r2 - r9 }
      ldmia r0!,{ r2 - r9 }
      stmia r1!,{ r2 - r9 }
    #endif
    // normal startup
    // get vbar
    mrc p15, 0, r1, c12, c0, 0
    ldr r0, =boot_start
    ldmia r0!,{ r2 - r9 }
    stmia r1!,{ r2 - r9 }
    ldmia r0!,{ r2 - r9 }
    stmia r1!,{ r2 - r9 }
    // clear data cache
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 1
    // ensure that it's written
    dsb
    // invalidate instruction cache
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 0
    // flush branch target cache
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 6
    // ensure actions to be done completely
    dsb
    isb

    // secure monitor call to reset scr
    smc #0
  #endif

  // enable fpu
  bl fpu_enable

  // prepare by platform
  bl boot_platform_prepare

  // load higher half start
  ldr r3, =start

  // set stack
  mov sp, r3

  // set program counter
  mov pc, r3
