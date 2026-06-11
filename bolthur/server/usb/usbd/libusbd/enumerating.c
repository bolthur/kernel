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

#include <errno.h>
// local includes
#include "../libusbd.h"

/**
 * @brief Enumerating flag
 */
static bool enumerating = false;

/**
 * @fn int usbd_enumerating_get(bool*)
 * @brief Get enumerating status
 * @param out output pointer
 * @return
 */
int usbd_enumerating_get( bool* out ) {
  if ( ! out ) {
    return EINVAL;
  }
  *out = enumerating;
  return 0;
}

/**
 * @fn int usbd_enumerating_set( bool )
 * @brief Set enumerating status
 * @param val enumerating value
 * @return
 */
int usbd_enumerating_set( const bool val ) {
  enumerating = val;
  return 0;
}
