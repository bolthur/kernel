
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

#include <avl.h>

/**
 * @brief Get the min object
 *
 * @param node current node
 * @param result result node
 * @return avl_node_ptr_t
 */
static avl_node_ptr_t get_min(
  avl_node_ptr_t node,
  avl_node_ptr_t result
) {
  // break if we reached the maximum
  if ( NULL == node ) {
    return result;
  }

  // just step to right for max entry
  return get_min( node->left, node );
}

/**
 * @brief Get min node of tree
 *
 * @param root node to get min value
 * @return avl_node_ptr_t found node or null if empty
 */
avl_node_ptr_t avl_get_min( const avl_node_ptr_t root ) {
  return get_min( root, NULL );
}
