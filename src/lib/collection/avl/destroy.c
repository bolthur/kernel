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

#include <stdlib.h>
#include <collection/avl.h>

/**
 * @brief Helper to destroy created tree
 *
 * @param tree
 */
void avl_destroy_tree( avl_tree_ptr_t tree ) {
  // check parameter
  if ( ! tree ) {
    return;
  }

  // loop as long a root node is existing
  while ( tree->root ) {
    // cache root node
    avl_node_ptr_t node = tree->root;
    // remove node from tree
    avl_remove_by_node( tree, node );
    // cleanup
    tree->cleanup( node );
  }
  // finally free tree itself
  free( tree );
}
