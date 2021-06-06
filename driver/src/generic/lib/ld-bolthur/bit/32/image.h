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

#include <stdint.h>
#include <stdbool.h>
#include <elf.h>
#include "../../list.h"

#ifndef __BIT_32_IMAGE_H__
#define __BIT_32_IMAGE_H__

struct elf_image {
  Elf32_Ehdr* header;
  size_t size;
  char* name;

  uintptr_t min_address;
  uintptr_t max_address;

  uint8_t* memory;
};
typedef struct elf_image elf_image_t;
typedef struct elf_image* elf_image_ptr_t;

#endif
