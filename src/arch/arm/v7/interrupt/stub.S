
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

.section .text

#define ASSEMBLER_FILE 1
#include <core/assembly.h>
#include <arch/arm/v7/cpu.h>
#include <arch/arm/v7/interrupt/handler.S>

EXPORT( interrupt_enable )
interrupt_enable:
  cpsie if
  bx lr

EXPORT( interrupt_disable )
interrupt_disable:
  mrs r0, cpsr
  cpsid if
  bx lr

// handler
IMPORT( vector_undefined_instruction_handler )
IMPORT( vector_svc_handler )
IMPORT( vector_prefetch_abort_handler )
IMPORT( vector_data_abort_handler )
IMPORT( vector_interrupt_handler )
IMPORT( vector_fast_interrupt_handler )

_vector_undefined_instruction_handler:
  exception_handler 4, vector_undefined_instruction_handler, CPSR_MODE_SUPERVISOR, vector_undefined, "none"

_vector_svc_handler:
  exception_handler 4, vector_svc_handler, CPSR_MODE_SUPERVISOR, vector_svc, "none"

_vector_prefetch_abort_handler:
  exception_handler 4, vector_prefetch_abort_handler, CPSR_MODE_SUPERVISOR, vector_prefetch_abort, "none"

_vector_data_abort_handler:
  exception_handler 8, vector_data_abort_handler, CPSR_MODE_SUPERVISOR, vector_data_abort, "none"

_vector_interrupt_handler:
  exception_handler 4, vector_interrupt_handler, CPSR_MODE_SUPERVISOR, vector_interrupt, "none"

_vector_fast_interrupt_handler:
  exception_handler 4, vector_fast_interrupt_handler, CPSR_MODE_SUPERVISOR, vector_fast_interrupt, "none"

EXPORT( interrupt_vector_table )
.balign 32
interrupt_vector_table:
  b start // reset
  b _vector_undefined_instruction_handler // undefined instruction
  b _vector_svc_handler // software interrupt
  b _vector_prefetch_abort_handler // prefetch abort
  b _vector_data_abort_handler // data abort
  nop // unused
  b _vector_interrupt_handler // irq
  b _vector_fast_interrupt_handler // fiq
