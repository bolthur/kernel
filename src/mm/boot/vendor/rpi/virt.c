
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
#include <mm/boot/arch/arm/virt.h>
#include <mm/kernel/arch/arm/virt.h>
#include <kernel/entry.h>

/**
 * @brief Method to setup short descriptor paging
 */
void SECTION( ".text.boot" )
boot_virt_vendor_setup( void ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    // map cpu local peripherals
    boot_virt_map(
      0x40000000,
      ( vaddr_t )0x40000000
    );
  #endif
}
