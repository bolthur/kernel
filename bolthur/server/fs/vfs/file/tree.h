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

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/bolthur.h>
#include "../../../../library/collection/avl/avl.h"
#include "../../../../library/collection/list/list.h"

#ifndef _FILE_TREE_H
#define _FILE_TREE_H

// forward declaration necessary due to circular referencing
typedef struct file_tree_node file_tree_node_t;
// structure itself
struct file_tree_node {
  avl_node_t node;
  pid_t pid;
  bool locked;
  char* name;
  struct stat* st;
  list_manager_t* children;
  list_manager_t* handle;
  file_tree_node_t* parent;
};

bool file_tree_setup(void);

#endif
