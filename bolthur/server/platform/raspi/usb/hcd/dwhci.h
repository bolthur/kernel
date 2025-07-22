/**
 * Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _DWHCI_H
#define _DWHCI_H

#include <stdint.h>
#include "response.h"

#define DWHCI_ENABLE_DEBUG 1

extern int fd_iomem;

response_t dwhci_query_vendor( uint32_t* destination );
response_t dwhci_power_on( void );
response_t dwhci_enable_global_interrupts( void );
response_t dwhci_disable_global_interrupts( void );
response_t dwhci_register_interrupt( void );
response_t dwhci_reset_device( void );
response_t dwhci_enable_common_interrupts( void );
response_t dwhci_read_core_cfg2( uint32_t* );
response_t dwhci_read_core_cfg( uint32_t* );
response_t dwhci_init_core( void );
response_t dwhci_read_host_cfg( uint32_t* );
response_t dwhci_core_flush_tx_fifo( uint32_t );
response_t dwhci_core_flush_rx_fifo( void );
response_t dwhci_write_host_port( uint32_t );
response_t dwhci_read_host_port( uint32_t* );
response_t dwhci_enable_host_interrupts( void );
response_t dwhci_init_host( void );
response_t dwhci_enable_root_port( void );
response_t dwhci_init( void );

#endif //_DWHCI_H
