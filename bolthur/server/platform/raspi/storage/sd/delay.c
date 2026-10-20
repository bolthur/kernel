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
void delay_us( const uint64_t us ) {
  const uint64_t ticks = _syscall_timer_tick_count() + ( ( _syscall_timer_frequency() * us + 999999ULL ) / 1000000ULL );
  while ( _syscall_timer_tick_count() < ticks ) {
    __asm__ __volatile__ ( "nop" : : : "memory" );
  }
}

static void custom_nanosleep( const struct timespec* rqtp ) {
  // get clock frequency
  size_t frequency = _syscall_timer_frequency();
  // calculate second timeout
  uint64_t timeout = ( uint64_t )( rqtp->tv_sec * frequency );
  // add nanosecond offset
  timeout += ( uint64_t )( ( double )rqtp->tv_nsec * ( double )frequency / 1000000000.0 );
  // add tick count to get an end time
  timeout += _syscall_timer_tick_count();
  // loop until timeout is reached
  while ( _syscall_timer_tick_count() < timeout ) {
    __asm__ __volatile__( "nop" );
  }
}

void delay_ms( uint64_t sleep_value ) {
  // variables
  struct timespec ts;
  // change to long
  long sleep_value_time = ( long )sleep_value;
  // prepare timespec structure
  ts.tv_sec = sleep_value_time / 1000;
  ts.tv_nsec = ( sleep_value_time % 1000 ) * 1000000;
  custom_nanosleep( &ts );
}

/**
 * @fn uint64_t delay_us_target(uint64_t)
 * @brief Calculate delay target
 * @param us microseconds to wait
 * @return
 */
uint64_t delay_us_target( const uint64_t us ) {
  const uint64_t frequency = _syscall_timer_frequency();
  return ( frequency * us + 999999ULL ) / 1000000ULL;
}
