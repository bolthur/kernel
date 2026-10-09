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

#ifndef _DELAY_H
#define _DELAY_H

void delay_us( uint64_t );
uint64_t delay_us_target( uint64_t );

#define DELAY_WAIT_TIMEOUT( condition, usec, result ) \
  { \
    bool loop = true; \
    do { \
      result = true; \
      uint64_t target = delay_us_target( usec ); \
      do { \
        if ( condition ) { \
          result = false; \
          loop = false; \
          break; \
        } \
      } while ( _syscall_timer_tick_count() < target ); \
    } while ( loop ); \
  }

#endif //_DELAY_H
