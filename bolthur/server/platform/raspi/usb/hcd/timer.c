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
#include "timer.h"

/**
 * @fn size_t timer_acquire(uint32_t)
 * @brief Acquire timer with delay of milliseconds
 * @param milliseconds milliseconds to wait
 * @return timer id
 * @exception EAGAIN in case timer was not possible to acquire
 */
size_t timer_acquire( const uint32_t milliseconds ) {
  // get clock frequency
  const size_t frequency = _syscall_timer_frequency();
  // translate into seconds
  const double seconds = (double)milliseconds / 1000.0;
  // calculate second timeout
  size_t timeout = ( size_t )( seconds * frequency );
  // add tick count to get an end time
  timeout += _syscall_timer_tick_count();
  // register timer
  return _syscall_timer_acquire( RPC_TIMER, timeout, false );
}
