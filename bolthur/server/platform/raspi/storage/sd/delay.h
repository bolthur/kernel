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
void delay_ms( uint64_t );
uint64_t delay_us_target( uint64_t );

#define DELAY_ITERATION( reg, condition, max_iteration, msec, result ) \
  { \
    result = false; \
    uint32_t iteration = max_iteration; \
    while ( mmio_read( reg ) & condition ) { \
      if ( ! iteration-- ) { \
        result = true; \
        break; \
      } \
      delay_ms( msec ); \
    } \
  }

#define DELAY_ITERATION_FALSE( reg, condition, max_iteration, msec, result ) \
  { \
    result = false; \
    uint32_t iteration = max_iteration; \
    while ( ! ( mmio_read( reg ) & condition ) ) { \
      if ( ! iteration-- ) { \
        result = true; \
        break; \
      } \
      delay_ms( msec ); \
    } \
  }

#define DELAY_ITERATION_FAILURE( reg, condition, failure, max_iteration, msec, result ) \
  { \
    result = false; \
    uint32_t iteration = max_iteration; \
    while ( mmio_read( reg ) & condition ) { \
      if ( mmio_read( reg ) & failure ) { \
        result = true; \
        break; \
      } \
      if ( ! iteration-- ) { \
        result = true; \
        break; \
      } \
      delay_ms( msec ); \
    } \
  }

#define DELAY_ITERATION_FAILURE_FALSE( reg, condition, failure, max_iteration, msec, result ) \
  { \
    result = false; \
    uint32_t iteration = max_iteration; \
    while ( ! ( mmio_read( reg ) & condition ) ) { \
      if ( mmio_read( reg ) & failure ) { \
        result = true; \
        break; \
      } \
      if ( ! iteration-- ) { \
        result = true; \
        break; \
      } \
      delay_ms( msec ); \
    } \
  }

#endif //_DELAY_H
