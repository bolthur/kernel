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

#if ! defined( __CORE_COMMON__ )
#define __CORE_COMMON__

#include <stdbool.h>
#include <stdint.h>
#include <core/task/process.h>

bool elf_check( uintptr_t );
bool elf_arch_check( uintptr_t );
uintptr_t elf_load( uintptr_t, task_process_ptr_t );
size_t elf_image_size( uintptr_t );

#endif
