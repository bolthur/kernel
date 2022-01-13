/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#if ! defined( _PLATFORM_RASPI_TIMER_H )
#define _PLATFORM_RASPI_TIMER_H

// free running counter incrementing at 1 MHz => Increments each microsecond
#define TIMER_FREQUENCY_HZ 1000000

// interrupts per second
#define TIMER_INTERRUPT_PER_SECOND 1

// Timer match bits
#define SYSTEM_TIMER_MATCH_0 ( 1 << 0 )
#define SYSTEM_TIMER_MATCH_1 ( 1 << 1 )
#define SYSTEM_TIMER_MATCH_2 ( 1 << 2 )
#define SYSTEM_TIMER_MATCH_3 ( 1 << 3 )

// timer interrupts
#define SYSTEM_TIMER_0_INTERRUPT ( 1 << 0 )
#define SYSTEM_TIMER_1_INTERRUPT ( 1 << 1 )
#define SYSTEM_TIMER_2_INTERRUPT ( 1 << 2 )
#define SYSTEM_TIMER_3_INTERRUPT ( 1 << 3 )


#endif
