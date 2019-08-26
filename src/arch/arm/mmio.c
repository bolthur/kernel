
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
#include <arch/arm/barrier.h>

/**
 * @brief Write to memory mapped I/O
 *
 * @param reg register/address to write to
 * @param data data to write
 */
void mmio_write( uint32_t reg, uint32_t data ) {
  barrier_data_mem();
  *( volatile uint32_t* )( ( uint32_t )reg ) = data;
  barrier_data_mem();
}

/**
 * @brief Read from memory mapped I/O
 *
 * @param reg register/address to read
 * @return uint32_t data from memory mapped I/O
 */
uint32_t mmio_read( uint32_t reg ) {
  barrier_data_mem();
  return *( volatile uint32_t* )( ( uint32_t )reg );
}
