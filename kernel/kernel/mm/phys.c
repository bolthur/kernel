
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#include <stddef.h>

#include "kernel/mm/phys.h"

/**
 * @brief Physical bitmap
 */
uintptr_t *phys_bitmap;

/**
 * @brief physical bitmap length set by vendor
 */
size_t phys_bitmap_length;

/**
 * @brief placement address starting at kernel end
 */
uintptr_t placement_address = ( uintptr_t )&__kernel_end;
