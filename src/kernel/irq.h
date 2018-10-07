
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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

#ifndef __KERNEL_IRQ__
#define __KERNEL_IRQ__

#include <stdint.h>
#include <stdbool.h>

typedef void (*irq_callback_t)(uint8_t irq, void *reg);
extern irq_callback_t irq_callback_map[];
extern irq_callback_t fast_irq_callback_map[];

void irq_init( void );
void irq_enable( void );
void irq_disable( void );
bool irq_validate_number( uint8_t num );
void irq_register_handler( uint8_t num, irq_callback_t func, bool fast );
int8_t irq_get_pending( bool fast );
irq_callback_t irq_get_handler( uint8_t num, bool fast );

#endif
