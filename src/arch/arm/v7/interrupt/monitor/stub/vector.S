
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

#include <core/assembly.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/interrupt/handler.S>

.section .text

// handler
IMPORT( monitor_unknown_handler )
IMPORT( monitor_prefetch_abort_handler )
IMPORT( monitor_data_abort_handler )
IMPORT( monitor_smc_handler )
IMPORT( monitor_interrupt_handler )
IMPORT( monitor_fast_interrupt_handler )

_monitor_unknown_handler:
  ex_handler 4, monitor_unknown_handler, CPSR_MODE_MONITOR, monitor_unknown, 0

_monitor_prefetch_abort_handler:
  ex_handler 4, monitor_prefetch_abort_handler, CPSR_MODE_MONITOR, monitor_prefetch_abort, 0

_monitor_data_abort_handler:
  ex_handler 8, monitor_data_abort_handler, CPSR_MODE_MONITOR, monitor_data_abort, 0

_monitor_smc_handler:
  ex_handler 4, monitor_smc_handler, CPSR_MODE_MONITOR, monitor_smc, 4

_monitor_interrupt_handler:
  ex_handler 4, monitor_interrupt_handler, CPSR_MODE_MONITOR, monitor_interrupt, 0

_monitor_fast_interrupt_handler:
  ex_handler 4, monitor_fast_interrupt_handler, CPSR_MODE_MONITOR, monitor_fast_interrupt, 0

EXPORT( monitor_vector_table )
.balign 32
monitor_vector_table:
  b _monitor_unknown_handler // unused
  b _monitor_unknown_handler // unused
  b _monitor_smc_handler // secure monitor call
  b _monitor_prefetch_abort_handler // prefetch abort
  b _monitor_data_abort_handler // data abort
  b _monitor_unknown_handler // unused
  b _monitor_interrupt_handler // irq
  b _monitor_fast_interrupt_handler // fiq
