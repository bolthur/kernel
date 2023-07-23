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

#include <sys/syslimits.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _HANDLE_H
#define _HANDLE_H

typedef struct {
  /** @brief number where handles start */
  int handle_start;
} handle_process_t;

typedef struct {
  /** @brief generated handle */
  int handle;
  /** @brief open flags */
  int flags;
  /** @brief mode */
  int mode;
  /** @brief current position */
  off_t pos;
  /** @brief file path */
  char path[ PATH_MAX ];
  /** @brief process handling file */
  pid_t handler;
  /** @brief stat information */
  struct stat info;
} handle_t;

handle_process_t* handle_generate_container( pid_t );
handle_t* handle_generate( void );

#endif
