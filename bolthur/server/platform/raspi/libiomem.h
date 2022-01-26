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

#define IOMEM_READ_MEMORY RPC_CUSTOM_START
#define IOMEM_WRITE_MEMORY IOMEM_READ_MEMORY + 1
#define IOMEM_MAILBOX IOMEM_WRITE_MEMORY + 1

struct iomem_read_request {
  uintptr_t offset;
  size_t len;
};
typedef struct iomem_read_request iomem_read_request_t;
typedef struct iomem_read_request* iomem_read_request_ptr_t;

struct iomem_write_request {
  uintptr_t offset;
  size_t len;
  uint32_t data[];
};
typedef struct iomem_write_request iomem_write_request_t;
typedef struct iomem_write_request* iomem_write_request_ptr_t;

#endif
