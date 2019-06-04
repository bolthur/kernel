
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

#include <stddef.h>

#include <string.h>
#include <kernel/panic.h>

/**
 * @brief Initialize virtual memory management
 */
void virt_vendor_init( void ) {
  PANIC( "To be implemented" );

  /**
   * Short descriptor initialization:
   *  - Setup new ttbr0 with 8KB size ( 2GB )
   *  - Setup new ttbr1 with 16KB size ( according to specs normal, when using ttbr0 and ttbr1 )
   *  - Set TTBCR.N to SD_TTBCR_N_TTBR0_2G
   *  - Set TTBCR.EAE to 0
   *  - Map peripherals to 0xF2000000 ( without caching )
   *  - Map framebuffer if built in to 0xF3000000 ( without caching )
   *  - Map additional with specific core mappings on rpi 2 and 3 ( without caching )
   *  - Replace ttbr0 and ttbr1 with new ones
   *  - Invalidate cache and tlb
   *  - Handle data barriers
   *  - Reset framebuffer base and peripheral base according to mappings
   */
}
