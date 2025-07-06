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

#include <stdint.h>
#include <stdlib.h>
#include "ht.h"

#include <assert.h>
#include <string.h>

/**
 * @fn uint64_t hash_key(const char*)
 * @brief Helper to create hash key
 * @param key key to create hash from
 * @return created fnv hash
 */
static uint64_t hash_key( const char* key ) {
  uint64_t hash = FNV_OFFSET;
  for ( const char* p = key; *p != '\0'; p++ ) {
    hash ^= (uint64_t)*p;
    hash *= FNV_PRIME;
  }
  return hash;
}

/**
 * @fn ht_set_entry(ht_entry_t*, size_t, const char*, void*, size_t*)
 * @brief Helper to set entry
 * @param entries entry list
 * @param capacity capacity
 * @param key key to set
 * @param value value to set
 * @param plength length
 * @return inserted key or NULL
 */
static const char* ht_set_entry(
  ht_entry_t* entries,
  size_t capacity,
  const char* key,
  void* value,
  size_t* plength
) {
  // handle invalid entries
  if ( ! entries ) {
    return NULL;
  }
  // create hash from key
  const uint64_t hash = hash_key( key );
  // get base index
  size_t index = ( size_t )( hash & ( uint64_t )( capacity - 1 ) );
  // loop while entries index key is valid
  while ( entries[index].key ) {
    // handle match of entry with key
    if ( strcmp( key, entries[index].key ) == 0 ) {
      // overwrite value
      entries[index].value = value;
      // return current key
      return entries[index].key;
    }
    // increment index
    index++;
    // handle capacity reached by resetting index
    if ( index >= capacity ) {
      index = 0;
    }
  }
  // handle length set
  if ( plength ) {
    // duplicate key
    key = strdup( key );
    // handle failure
    if ( ! key ) {
      return NULL;
    }
    // increment length
    ( *plength )++;
  }
  // populate key and value of found index
  entries[index].key = key;
  entries[index].value = value;
  // return key
  return key;
}

/**
 * @fn bool ht_expand(ht_t*)
 * @brief Method to expand hash table
 * @param table table to expand
 * @return true on success, else false
 */
static bool ht_expand( ht_t* table ) {
  // calculate new capacity
  const size_t new_capacity = table->capacity * 2;
  // handle new capacity smaller than table capacity ( overflow )
  if ( new_capacity < table->capacity ) {
    // return failure
    return false;
  }
  // allocate new entries
  ht_entry_t* new_entries = calloc( new_capacity, sizeof( ht_entry_t ) );
  // handle failure
  if ( ! new_entries ) {
    return false;
  }
  // loop through all existing elements
  for ( size_t i = 0; i < table->capacity; i++ ) {
    // handle key set
    if ( table->entries[ i ].key ) {
      // call set entry without increment of length
      ht_set_entry( new_entries, new_capacity, table->entries[ i ].key,
        table->entries[ i ].value, NULL );
    }
  }
  // free existing entries
  free( table->entries );
  // populate entries and new capacity
  table->entries = new_entries;
  table->capacity = new_capacity;
  // return success
  return true;
}

/**
 * @fn ht_t* ht_create(void)
 * @brief Method to create hash table
 * @return Created hash table or null
 */
ht_t* ht_create( void ) {
  // allocate space for hash table structure
  ht_t* table = malloc( sizeof( ht_t ) );
  // handle error
  if ( ! table ) {
    // return null
    return NULL;
  }
  // prefill length and capacity
  table->length = 0;
  table->capacity = INITIAL_CAPACITY;
  // allocate space for entries
  table->entries = calloc( table->capacity, sizeof( ht_entry_t ) );
  // handle error
  if ( ! table->entries ) {
    // free table again
    free( table );
    // return null
    return NULL;
  }
  // return allocated table
  return table;
}

/**
 * @fn void ht_destroy(ht_t*)
 * @brief Method to destroy hash table
 * @param table Hash table to destroy
 */
void ht_destroy( ht_t* table ) {
  // handle no valid table
  if ( ! table ) {
    return;
  }
  // free allocated keys
  for ( size_t i = 0; i < table->capacity; i++ ) {
    // free key
    free( ( void* )table->entries[i].key );
  }
  // free entries and structure
  free( table->entries );
  free( table );
}

/**
 * @fn void* ht_get(ht_t*, const char*)
 * @brief Method to get entry from table
 * @param table table to lookup
 * @param key key to lookup
 * @return found value or NULL
 */
void* ht_get( const ht_t* table, const char* key ) {
  // handle no table or no key
  if ( ! table || ! key ) {
    return NULL;
  }
  // create hash from key
  const uint64_t hash = hash_key( key );
  // get base index
  size_t index = ( size_t )( hash & ( uint64_t )( table->capacity - 1 ) );
  // loop while entries index key is valid
  while ( table->entries[ index ].key ) {
    // handle match
    if ( strcmp( key, table->entries[ index ].key ) == 0 ) {
      // return set value
      return table->entries[ index ].value;
    }
    // increment index
    index++;
    // reset index if greater than capacity
    if ( index >= table->capacity ) {
      index = 0;
    }
  }
  // return null if not found
  return NULL;
}

/**
 * @fn const char* ht_set(ht_t*, const char*, void*)
 * @brief Method to set hash table entry
 * @param table table to update
 * @param key key to set
 * @param value value to set
 * @return added key or NULL
 */
const char* ht_set( ht_t* table, const char* key, void* value ) {
  // handle invalid
  if ( ! value || ! table || ! key ) {
    return NULL;
  }
  // expand hash table if necessary
  if ( table->length >= table->capacity / 2 && ! ht_expand( table ) ) {
    return NULL;
  }
  // set entry and update it
  return ht_set_entry( table->entries, table->capacity, key, value, &table->length );
}

/**
 * @fn void ht_unset(ht_t*, const char*)
 * @brief Method to unset hash map key
 * @param table table to unset key from
 * @param key key to unset
 */
void ht_unset( ht_t* table, const char* key ) {
  // handle no table or no key
  if ( ! table || ! key ) {
    return;
  }
  // create hash from key
  const uint64_t hash = hash_key( key );
  // get base index
  size_t index = ( size_t )( hash & ( uint64_t )( table->capacity - 1 ) );
  // loop while entries index key is valid
  while ( table->entries[ index ].key ) {
    // handle match
    if ( strcmp( key, table->entries[ index ].key ) == 0 ) {
      // free up key
      free( ( void* )table->entries[ index ].key );
      table->entries[ index ].key = NULL;
      table->length--;
      // return early
      return;
    }
    // increment index
    index++;
    // reset index if greater than capacity
    if ( index >= table->capacity ) {
      index = 0;
    }
  }
}

/**
 * @fn size_t ht_length(const ht_t*)
 * @brief Wrapper to get hash table length
 * @param table hash table to get length of
 * @return hash table length
 */
size_t ht_length( const ht_t* table ) {
  return table->length;
}

/**
 * @fn hti_t ht_iterator(ht_t*)
 * @brief Wrapper to get iterator
 * @param table table to get iterator for
 * @return iterator
 */
hti_t ht_iterator( ht_t* table ) {
  static hti_t it;
  it.table = table;
  it.index = 0;
  return it;
}

/**
 * @fn bool ht_next(hti_t*)
 * @brief Get next entry from iterator
 * @param it iterator
 * @return true when next entry is existing, else false
 */
bool ht_next( hti_t* it ) {
  // cache table
  const ht_t* table = it->table;
  // loop while index reaches capacity
  while ( it->index < it->table->capacity ) {
    // get current index
    const size_t i = it->index;
    // increment index
    it->index++;
    // handle key set
    if ( table->entries[i].key ) {
      // populate key and value of iterator
      it->key = table->entries[i].key;
      it->value = table->entries[i].value;
      // return success
      return true;
    }
  }
  // return failure
  return false;
}
