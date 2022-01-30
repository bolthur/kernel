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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/bolthur.h>

#if ! defined( _LIBIOMEM_H )
#define _LIBIOMEM_H

#define IOMEM_MAILBOX RPC_CUSTOM_START
#define IOMEM_MMIO IOMEM_MAILBOX + 1

enum mmio_action {
  IOMEM_MMIO_ACTION_LOOP_EQUAL = 1,
  IOMEM_MMIO_ACTION_LOOP_NOT_EQUAL,
  IOMEM_MMIO_ACTION_READ,
  IOMEM_MMIO_ACTION_READ_OR,
  IOMEM_MMIO_ACTION_READ_AND,
  IOMEM_MMIO_ACTION_WRITE,
  IOMEM_MMIO_ACTION_WRITE_PREVIOUS_READ,
  IOMEM_MMIO_ACTION_WRITE_OR_PREVIOUS_READ,
  IOMEM_MMIO_ACTION_WRITE_AND_PREVIOUS_READ,
  IOMEM_MMIO_ACTION_DELAY,
  IOMEM_MMIO_ACTION_SLEEP,
};
typedef enum mmio_action mmio_action_t;

enum mmio_shift {
  IOMEM_MMIO_SHIFT_NONE = 0,
  IOMEM_MMIO_SHIFT_LEFT,
  IOMEM_MMIO_SHIFT_RIGHT,
};
typedef enum mmio_shift mmio_shift_t;

enum mmio_sleep {
  IOMEM_MMIO_SLEEP_NONE = 0,
  IOMEM_MMIO_SLEEP_MILLISECONDS,
  IOMEM_MMIO_SLEEP_SECONDS,
};
typedef enum mmio_sleep mmio_sleep_t;

struct iomem_mmio_entry {
  // action type
  mmio_action_t type;
  // memory offset
  uint32_t offset;
  // value ( usage depending on action type )
  uint32_t value;
  // shift type and shift bits to be applied
  mmio_shift_t shift_type;
  uint32_t shift_value;
  // value to be used for loop check
  uint32_t loop_and;
  uint32_t loop_max_iteration;
  // optional second value for loop
  uint32_t loop_offset2;
  uint32_t loop_value2;
  uint32_t loop_and2;
  // sleep type and amount to sleep
  mmio_sleep_t sleep_type;
  uint32_t sleep;
};
typedef struct iomem_mmio_entry iomem_mmio_entry_t;
typedef struct iomem_mmio_entry* iomem_mmio_entry_ptr_t;
typedef struct iomem_mmio_entry iomem_mmio_entry_array_t[];

#endif
