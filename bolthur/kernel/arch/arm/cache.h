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

#if ! defined( _ARCH_ARM_CACHE_H )
#define _ARCH_ARM_CACHE_H

void cache_enable_stub( bool* );
void cache_invalidate_instruction_cache( void );
void cache_flush_prefetch( void );
void cache_flush_branch_target( void );
void cache_invalidate_data( void );
void cache_clean_data( void );
void cache_invalidate_save( void );

#endif
