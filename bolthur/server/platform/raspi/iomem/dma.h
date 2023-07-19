/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include <stdbool.h>
#include <stdint.h>

#ifndef _DMA_H
#define _DMA_H

typedef struct dma_control_block {
  uint32_t transfer_information;
  uint32_t source_address;
  uint32_t destination_address;
  uint32_t transfer_length;
  uint32_t stride;
  uint32_t next_control_block;
  uint32_t reserved[ 2 ];
} dma_control_block_t;

int dma_block_to_phys( uintptr_t* );
int dma_block_prepare( void );
void dma_block_dump( void );

int dma_block_set_address( uint32_t, uint32_t );
int dma_block_set_transfer_length( uint32_t );
int dma_block_set_stride( uint32_t );
int dma_block_set_next( uint32_t );

int dma_block_transfer_info_source_increment( bool );
int dma_block_transfer_info_destination_increment( bool );
int dma_block_transfer_info_wait_response( bool );
int dma_block_transfer_info_src_width( bool );
int dma_block_transfer_info_dest_width( bool );
int dma_block_transfer_info_src_dreq( bool );
int dma_block_transfer_info_dest_dreq( bool );
int dma_block_transfer_info_interrupt_enable( bool );
int dma_block_transfer_info_permap( uint32_t );

void dma_init( void );
int dma_start( void );
int dma_wait( void );
int dma_finish( void );
void dma_dump( void );

#endif
