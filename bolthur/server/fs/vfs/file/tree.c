/**
 * Copyright (C) 2018 - 2023 bolthur project.
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

#include "tree.h"

avl_tree_t* file_tree = NULL;

/**
 * @brief Compare handle callback necessary for avl tree insert / delete
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_node(
  __unused const avl_node_t* node_a,
  __unused const avl_node_t* node_b
) {
  /// FIXME: REPLACE WITH LOGIC
  return 0;
}

/**
 * @brief Lookup handle callback necessary for avl tree search operations
 *
 * @param node current node
 * @param value value that is looked up
 * @return int32_t
 */
static int32_t lookup_node(
  const avl_node_t* node,
  const void* value
) {
  return avl_default_lookup( node, value );
}

/**
 * @fn void cleanup_handle(avl_node_t*)
 * @brief handle cleanup
 *
 * @param node
 */
static void cleanup_node( avl_node_t* node ) {
  avl_default_cleanup( node );
}

/**
 * @fn bool file_tree_setup(void)
 * @brief Method to setup file tree
 *
 * @return
 */
bool file_tree_setup(void) {
  // handle already initialized
  if ( file_tree ) {
    return true;
  }
  // setup avl tree
  file_tree = avl_create_tree( compare_node, lookup_node, cleanup_node );
  // return success if creation went well
  return file_tree;
}
