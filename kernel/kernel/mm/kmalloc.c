
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @brief External kernel start defined within linker
 */
extern uint32_t __kernel_start;

/**
 * @brief External kernel end defined within linker
 */
extern uint32_t __kernel_end;

/**
 * @brief placement address starting at kernel end
 */
uint32_t placement_address = ( uint32_t )&__kernel_end;

/**
 * @brief Simple kmalloc
 *
 * @param size amount of bytes to get
 * @return uint32_t start address of aligned memory
 */
uint32_t kmalloc( size_t size ) {
  uint32_t tmp = placement_address;
  placement_address += size;
  return tmp;
}
