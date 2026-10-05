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

#include "../cpu/pool.h"
#include "backup.h"
#include "data.h"
#include "pool.h"

/**
 * @fn void rpc_backup_destroy(rpc_backup_t*)
 * @brief backup to destroy
 *
 * @param backup
 */
void rpc_backup_destroy( rpc_backup_t* backup ) {
  // handle invalid
  if ( ! backup ) {
    return;
  }
  // free context
  if ( backup->context ) {
    cpu_pool_push( backup->context );
  }
  // push again back on pool
  rpc_pool_push( backup );
}
