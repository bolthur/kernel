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

#include <elf.h>
#include "../../image.h"

/**
 * @fn bool image_validate(void*)
 * @brief Image validation for 32 bit architecture
 *
 * @param img
 * @return
 */
bool image_validate( void* img ) {
  Elf32_Ehdr* header = ( Elf32_Ehdr* )img;
  // handle invalid
  if ( ! header ) {
    return false;
  }
  // check magic
  if (
    ELFMAG0 != header->e_ident[ EI_MAG0 ]
    || ELFMAG1 != header->e_ident[ EI_MAG1 ]
    || ELFMAG2 != header->e_ident[ EI_MAG2 ]
    || ELFMAG3 != header->e_ident[ EI_MAG3 ]
  ) {
    // return error
    return false;
  }
  // handle wrong architecture
  if ( ELFCLASS32 != header->e_ident[ EI_CLASS ] ) {
    return false;
  }
  // architecture related checks
  if ( ! image_validate_machine( img ) ) {
    return false;
  }
  // ensure program header existence
  if ( 0 == header->e_phentsize ) {
    return false;
  }
  // ensure section header existence
  if ( 0 == header->e_shentsize ) {
    return false;
  }
  // return success
  return true;
}
