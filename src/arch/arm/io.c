
/**
 * Copyright (C) 2018 - 2021 bolthur project.
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
#include <core/io.h>

/**
 * @brief Read 8 bit from memory mapped I/O
 *
 * @param port port to read
 * @return uint8_t received value
 */
uint8_t io_in8( uint32_t port ) {
  return ( uint8_t )io_in32( port );
}

/**
 * @brief Write 8 bit to memory mapped I/O
 *
 * @param port port to read
 * @param val value to write
 */
void io_out8( uint32_t port, uint8_t val ) {
  io_out32( port, ( uint32_t )val );
}

/**
 * @brief Read 16 bit from memory mapped I/O
 *
 * @param port port to read
 * @return uint16_t received value
 */
uint16_t io_in16( uint32_t port ) {
  return ( uint16_t )io_in32( port );
}

/**
 * @brief Write 16 bit to memory mapped I/O
 *
 * @param port port to read
 * @param val value to write
 */
void io_out16( uint32_t port, uint16_t val ) {
  io_out32( port, ( uint32_t )val );
}

/**
 * @brief Read 32 bit from memory mapped I/O
 *
 * @param port port to read
 * @return uint32_t received value
 */
uint32_t io_in32( uint32_t port ) {
  barrier_data_mem();
  return *( volatile uint32_t* )( port );
}

/**
 * @brief Write 32 bit to memory mapped I/O
 *
 * @param port port to read
 * @param val value to write
 */
void io_out32( uint32_t port, uint32_t val ) {
  barrier_data_mem();
  *( volatile uint32_t* )( port ) = val;
  barrier_data_mem();
}
