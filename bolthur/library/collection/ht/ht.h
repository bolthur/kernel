/**
* Copyright (C) 2018 - 2025 bolthur project.
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

#ifndef _HT_H
#define _HT_H

#include <stdbool.h>
#include <stddef.h>

#define INITIAL_CAPACITY 16

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

typedef struct {
  const char* key;
  void* value;
} ht_entry_t;

typedef struct {
  ht_entry_t* entries;
  size_t capacity;
  size_t length;
} ht_t;

typedef struct hti {
  const char* key;
  void* value;
  ht_t* table;
  size_t index;
} hti_t;

ht_t* ht_create( void );
void ht_destroy( ht_t* );
void* ht_get( const ht_t*, const char* );
const char* ht_set( ht_t*, const char*, void* );
void ht_unset( ht_t*, const char* );
size_t ht_length( const ht_t* );
hti_t ht_iterator( ht_t* );
bool ht_next( hti_t* );

#endif
