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

#include <stdint.h>
#include <assert.h>

#ifndef _FAT_FSINFO_H
#define _FAT_FSINFO_H

#pragma pack(push, 1)

// fat32 fs info
typedef struct {
  uint32_t lead_signature;
  uint8_t reserved[ 480 ];
  uint32_t signature;
  uint32_t known_free_cluster_count;
  uint32_t available_cluster_start;
  uint8_t reserved2[ 12 ];
  uint32_t trail_signature;
} fat32_fsinfo_t;

static_assert( 512 == sizeof( fat32_fsinfo_t ), "invalid fat32_fsinfo_t size!" );

#pragma pack(pop)

#endif
