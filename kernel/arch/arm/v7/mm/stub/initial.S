
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

#define ASSEMBLER_FILE 1

#include "kernel/entry.h"
#include "arch/arm/v7/mm/virt.h"

#if defined( IS_HIGHER_HALF )
  // reserve initial page directory and page table aligned by power of 2
  .comm initial_page_directory, VIRT_INITIAL_PAGE_DIRECTORY_SIZE, 2
  .comm initial_page_table, VIRT_INITIAL_PAGE_TABLE_SIZE, 2

  .global virt_setup_initial
  virt_setup_initial:
    bx lr
#endif
