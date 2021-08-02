/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#if ! defined( __LIB_ENDIAN__ )
#define __LIB_ENDIAN__

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  #define htole16(x) (x)
  #define htole32(x) (x)
  #define htole64(x) (x)

  #define htobe16(x) __builtin_bswap16(x)
  #define htobe32(x) __builtin_bswap32(x)
  #define htobe64(x) __builtin_bswap64(x)

  #define le16toh(x) (x)
  #define le32toh(x) (x)
  #define le64toh(x) (x)

  #define be16toh(x) __builtin_bswap16(x)
  #define be32toh(x) __builtin_bswap32(x)
  #define be64toh(x) __builtin_bswap64(x)
#else
  #define htole16(x) __builtin_bswap16(x)
  #define htole32(x) __builtin_bswap32(x)
  #define htole64(x) __builtin_bswap64(x)

  #define htobe16(x) (x)
  #define htobe32(x) (x)
  #define htobe64(x) (x)

  #define le16toh(x) __builtin_bswap16(x)
  #define le32toh(x) __builtin_bswap32(x)
  #define le64toh(x) __builtin_bswap64(x)

  #define be16toh(x) (x)
  #define be32toh(x) (x)
  #define be64toh(x) (x)
#endif

#endif
