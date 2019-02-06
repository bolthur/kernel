
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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

#if ! defined( __LIBC_STDLIB__ )
#define __LIBC_STDLIB__

#include <stdint.h>
#include <stdbool.h>

#define RAND_MAX 32767

#if defined( __cplusplus )
extern "C" {
#endif

char *itoa( int32_t, char*, int32_t, bool );
int rand( void );
void srand( unsigned int );
char *utoa( uint32_t, char*, int32_t, bool );

#if defined( __cplusplus )
}
#endif

#endif
