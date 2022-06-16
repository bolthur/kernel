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

#ifndef _LIB_INTTYPES_H
#define _LIB_INTTYPES_H

#include <inttypes.h>

/// following is necessary for freestanding built and cppcheck to work both
#if defined( __PRI64 ) && ! defined( PRId64 )
  #define PRId64 __PRI64(d)
  #define PRIi64 __PRI64(i)
  #define PRIo64 __PRI64(o)
  #define PRIu64 __PRI64(u)
  #define PRIx64 __PRI64(x)
  #define PRIX64 __PRI64(X)
#endif
#if defined( __SCN64 ) && ! defined( SCNd64 )
  #define SCNd64 __SCN64(d)
  #define SCNi64 __SCN64(i)
  #define SCNo64 __SCN64(o)
  #define SCNu64 __SCN64(u)
  #define SCNx64 __SCN64(x)
#endif

#endif
