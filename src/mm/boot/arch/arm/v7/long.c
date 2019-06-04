
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

#include <mm/kernel/arch/arm/virt.h>
#include <kernel/entry.h>
#include <mm/boot/arch/arm/v7/short.h>

/**
 * @brief Helper to setup initial paging with large page address extension
 *
 * @todo Add initial mapping for paging with lpae
 * @todo Remove call for setup short paging
 */
void SECTION( ".text.boot" ) boot_setup_long_vmm( paddr_t max_memory ) {
  boot_setup_short_vmm( max_memory );
}
