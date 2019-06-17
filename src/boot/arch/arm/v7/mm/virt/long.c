
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

#include <arch/arm/mm/virt.h>
#include <kernel/entry.h>
#include <boot/arch/arm/v7/mm/virt/short.h>

/**
 * @brief Helper to setup initial paging with large page address extension
 *
 * @todo Add initial mapping for paging with lpae
 * @todo Remove call for setup short paging
 */
void SECTION( ".text.boot" )
boot_virt_setup_long( paddr_t max_memory ) {
  boot_virt_setup_short( max_memory );
}

/**
 * @brief Method to perform identity nap
 */
void SECTION( ".text.boot" )
boot_virt_map_long( paddr_t phys, vaddr_t virt ) {
  boot_virt_map_short( phys, virt );
}
