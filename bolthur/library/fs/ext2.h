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

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "ext2/superblock.h"
#include "ext2/blockgroup.h"
#include "ext2/inode.h"
#include "ext2/directory.h"

#ifndef _EXT2_H
#define _EXT2_H

typedef bool (*device_read_t)(uint32_t* dest, size_t size, uint32_t start );

int32_t ext2_superblock_read( device_read_t, ext2_superblock_t*, uint32_t );

#endif
