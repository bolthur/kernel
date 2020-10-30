
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <collection/avl.h>

/**
 * @brief Get the max object
 *
 * @param node current node
 * @param result result node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t get_max(
  avl_node_ptr_t node,
  avl_node_ptr_t result
) {
  // break if we reached the maximum
  if ( ! node ) {
    return result;
  }

  // just step to right for max entry
  return get_max( node->right, node );
}

/**
 * @brief Get max node of tree
 *
 * @param root root to get max node
 * @return avl_node_ptr_t found node or null if empty
 */
avl_node_ptr_t avl_get_max( const avl_node_ptr_t root ) {
  return get_max( root, NULL );
}
