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

#include <stdlib.h>
#include <string.h>
#include "../../firmware.h"

/**
 * @brief Firmware name
 */
#define FIRMWARE_NAME "firmware-1.20250430"

/**
 * @brief Firmware url
 */
#define FIRMWARE_URL "https://github.com/raspberrypi/firmware/archive/refs/tags/1.20250430.tar.gz"

/**
 * @brief Function to get firmware name
 * @return
 */
char* firmware_get_name( void ) {
  char* name = malloc( strlen( FIRMWARE_NAME ) + 1 );
  if ( ! name ) {
    return nullptr;
  }
  strcpy( name, FIRMWARE_NAME );
  return name;
}

/**
 * @brief Function to get firmware url
 * @return
 */
char* firmware_get_url( void ) {
  char* url = malloc( strlen( FIRMWARE_URL ) + 1 );
  if ( ! url ) {
    return nullptr;
  }
  strcpy( url, FIRMWARE_URL );
  return url;
}
