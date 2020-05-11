
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

#if ! defined( __PLATFORM_RPI_PERIPHERAL__ )
#define __PLATFORM_RPI_PERIPHERAL__

#include <stdint.h>

typedef enum {
  PERIPHERAL_GPIO,
  PERIPHERAL_LOCAL,
} peripheral_type_t;

void peripheral_base_set( uintptr_t, peripheral_type_t );
uintptr_t peripheral_base_get( peripheral_type_t );
uintptr_t peripheral_end_get( peripheral_type_t );

#endif
