/**
 * Copyright (C) 2018 - 2022 bolthur project.
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

#include <string.h>
#include <stdlib.h>
#include "../ext.h"

/**
 * @fn ext_directory_entry_t ext_directory_get_directory*(ext_inode_t*, const char*)
 * @brief Get directory within inode
 *
 * @param inode
 * @param name
 * @return
 */
ext_directory_entry_t* ext_directory_get_directory(
  ext_inode_t* inode,
  const char* name
) {
  ext_directory_entry_t* entry = NULL;
  ext_directory_entry_t* result = NULL;
  // allocate buffer
  uint8_t* buffer = malloc( inode->inode->i_size );
  if ( ! buffer ) {
    return NULL;
  }
  // clear buffer
  memset( buffer, 0, inode->inode->i_size );
  // read inode data
  if ( ! ext_inode_read_data( inode, 0, inode->inode->i_size, buffer ) ) {
    free( buffer );
    return NULL;
  }
  // loop while position is smaller than size
  uint32_t position = 0;
  while ( position < inode->inode->i_size ) {
    // get entry from buffer
    entry = ( ext_directory_entry_t* )( &buffer[ position ] );
    // increment position
    position += entry->rec_len;
    // handle unknown
    if ( entry->inode == EXT_FT_UNKNOWN ) {
      continue;
    }
    // handle no match by length
    if ( strlen( name ) != entry->name_len ) {
      continue;
    }
    // check for match
    if ( 0 == strncmp( name, entry->name, entry->name_len ) ) {
      // allocate result
      result = malloc( entry->rec_len );
      if ( ! result ) {
        break;
      }
      // clear out space
      memset( result, 0, entry->rec_len );
      // copy over content
      memcpy( result, entry, entry->rec_len );
      // break loop
      break;
    }
    // handle invalid length
    if ( 0 == entry->rec_len ) {
      break;
    }
  }
  // free buffer
  free( buffer );
  // return result
  return result;
}

/**
 * @fn bool ext_directory_get_inode(ext_fs_t*, const char*, ext_inode_t*)
 * @brief Get directory inode
 *
 * @param fs
 * @param path
 * @param inode
 * @return
 */
bool ext_directory_get_inode(
  ext_fs_t* fs,
  const char* path,
  ext_inode_t* inode
) {
  const char* delim = "/";
  uint32_t parent_inode_number = EXT_ROOT_INO;
  ext_inode_t parent_inode;
  ext_directory_entry_t* entry;
  // duplicate path
  char* path_dup = strdup( path );
  // get first part of path
  char* part = strtok( path_dup, delim );
  // loop while part is valid
  while ( part ) {
    // if part has length, try to read inode
    if ( 0 != strlen( part ) ) {
      // read parent inode
      if ( ! ext_inode_read_inode( fs, parent_inode_number, &parent_inode ) ) {
        return false;
      }
      // get directory from inode
      entry = ext_directory_get_directory( &parent_inode, part );
      // release inode again
      if ( ! ext_inode_release_inode( &parent_inode ) ) {
        if ( entry ) {
          free( entry );
        }
        return false;
      }
      // handle no directory entry found
      if ( ! entry ) {
        return false;
      }
      // adjust parent inode
      parent_inode_number = entry->inode;
      // free directory again
      free( entry );
    }
    // get next part
    part = strtok( NULL, delim );
  }
  // free duplicate
  free( path_dup );
  // return inode
  return ext_inode_read_inode( fs, parent_inode_number, inode );
}
