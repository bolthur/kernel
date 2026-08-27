/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include <sys/bolthur.h>
#include "delay.h"

/**
 * @brief Helper to delay
 * @param us
 */
void delay_us( const uint32_t us ) {
  const uint64_t frequency = _syscall_timer_frequency();
  const uint64_t ticks =
      (frequency * (uint64_t)us + 999999ULL) / 1000000ULL;
  const uint64_t start = _syscall_timer_tick_count();
  while ((_syscall_timer_tick_count() - start) < ticks) {
    __asm__ volatile ("nop");
  }
}
