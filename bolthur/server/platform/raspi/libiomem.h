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

#define IOMEM_MMIO_LOOP_EQUAL 1
#define IOMEM_MMIO_LOOP_NOT_EQUAL 2
#define IOMEM_MMIO_READ 3
#define IOMEM_MMIO_WRITE 4
#define IOMEM_MMIO_WRITE_OR_PREVIOUS_READ 5
#define IOMEM_MMIO_DELAY 6
#define IOMEM_MMIO_SLEEP 7

#define IOMEM_MMIO_SHIFT_LEFT 1
#define IOMEM_MMIO_SHIFT_RIGHT 2

struct iomem_mmio_entry {
  uint32_t type;
  uint32_t offset;
  uint32_t value;
  uint32_t shift_type;
  uint32_t shift_value;
};
typedef struct iomem_mmio_entry iomem_mmio_entry_t;
typedef struct iomem_mmio_entry* iomem_mmio_entry_ptr_t;
typedef struct iomem_mmio_entry iomem_mmio_entry_array_t[];

#endif
