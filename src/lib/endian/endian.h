
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

#ifndef __LIBENDIAN__
#define __LIBENDIAN__

#include <stdint.h>

#if defined( __cplusplus )
extern "C" {
#endif

uint8_t uint8_little_to_big( const void* );
uint16_t uint16_little_to_big( const void* );
uint32_t uint32_little_to_big( const void* );
uint64_t uint64_little_to_big( const void* );
float float_little_to_big( const void* );
double double_little_to_big( const void* );

#if defined( __cplusplus )
}
#endif

#endif
