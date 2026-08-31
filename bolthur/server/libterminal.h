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

#ifndef _LIBTERMINAL_H
#define _LIBTERMINAL_H

#include <sys/bolthur.h>

#define TERMINAL_IN_START RPC_CUSTOM_START
#define TERMINAL_OUT_START TERMINAL_IN_START + 1
#define TERMINAL_ERR_START TERMINAL_OUT_START + 1

typedef struct {
  size_t len;
  size_t shm_id;
  char terminal[];
} terminal_write_request_t;

#endif
