
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#if !defined( __CORE_SMP_LOCK__ )
#define __CORE_SMP_LOCK__

#include <stdint.h>

typedef volatile int32_t smp_lock_mutex_t;

#define SMP_LOCK_MUTEX_LOCKED 1
#define SMP_LOCK_MUTEX_RELEASED 0

void smp_lock_mutex_acquire( smp_lock_mutex_t* );
void smp_lock_mutex_release( smp_lock_mutex_t* );

#endif