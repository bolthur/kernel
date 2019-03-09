
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

#include <stdint.h>

#include "lib/stdc/stdio.h"
#include "lib/tar/tar.h"

#include "kernel/kernel/panic.h"
#include "kernel/kernel/mm/phys.h"
#include "kernel/vendor/rpi/platform.h"
#include "kernel/vendor/rpi/mailbox/property.h"

/**
 * @brief Boot parameter data set during startup
 */
platform_loader_parameter_t loader_parameter_data;

/**
 * @brief Platform depending initialization routine
 *
 * @todo Move placement address beyond initrd
 */
void platform_init( void ) {
}
