
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

/**
 * @brief Method to setup short descriptor paging
 */
void SECTION( ".text.boot" )
boot_vendor_setup_short_vmm( sd_context_total_t* ctx ) {
  #if defined( BCM2709 ) || defined( BCM2710 )
    uint32_t x;
    x = ( 0x40000000 >> 20 );
    ctx->section[ x ].data.type = SD_TTBR_TYPE_SECTION;
    ctx->section[ x ].data.execute_never = 0;
    ctx->section[ x ].data.access_permision_0 = SD_MAC_APX0_FULL_RW;
    ctx->section[ x ].data.frame = x & 0xFFF;
  #endif
}
