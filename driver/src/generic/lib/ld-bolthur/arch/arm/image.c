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

#include "../../image.h"
#include <elf.h>

/**
 * @fn bool image_validate_header(void*)
 * @brief Method to validate header
 *
 * @param img
 * @return
 */
bool image_validate_machine( void* img ) {
  Elf32_Ehdr* header = ( Elf32_Ehdr* )img;
  // check machine
  if ( EM_ARM != header->e_machine ) {
    return false;
  }
  return true;
}
