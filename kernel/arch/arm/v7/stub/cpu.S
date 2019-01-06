
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
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

// Helper to stop all cpus except the first one during startup
.global arch_cpu_reset
arch_cpu_reset:
  // check for hypervisor mode and switch back to supervisor mode for rpi2 and rpi3
  // Return current CPU ID (0..3)
  cpsid if // Disable IRQ & FIQ
  mrc p15, 0, r3, c0, c0, 5 // r3 = Multiprocessor Affinity Register (MPIDR)
  ands r3, #3 // r0 = CPU ID (Bits 0..1)
  bne _hlt // If (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)
  beq _arch_cpu_reset // If equal branch to normal startup
_hlt:
  wfe
  b _hlt
_arch_cpu_reset:
  bx lr

// FIXME: Add logic
.global invalidate_l1_cache
invalidate_l1_cache:
  bx lr

// FIXME: Add logic
.global start_l1_cache
start_l1_cache:
  bx lr

// FIXME: Add logic
.global stop_l1_cache
stop_l1_cache:
  bx lr

// FIXME: Add logic
.global invalidate_l2_cache
invalidate_l2_cache:
  bx lr

// FIXME: Add logic
.global start_l2_cache
start_l2_cache:
  bx lr

// FIXME: Add logic
.global stop_l2_cache
stop_l2_cache:
  bx lr
