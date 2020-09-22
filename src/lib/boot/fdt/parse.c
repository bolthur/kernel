
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

#include <boot/fdt.h>
#include <boot/string.h>

/**
 * @brief Internal helper to parse recursively for address at property
 *
 * @param path path to property
 * @param offset possible offset, when property contains 2 or more addresses
 * @param lex starting structure
 * @param string string informations like names
 * @return uintptr_t
 *
 * @todo extract and consider #address-cells and #size-cells correctly
 * @todo move recursive walk through flattened device tree into function with callback used for return
 */
static uintptr_t __bootstrap fdt_parse_address_recursive(
  const char* path,
  size_t offset,
  uintptr_t lex,
  uintptr_t string
) {
  // skip leading slash
  if ( '/' == *path ) {
    path++;
  }

  // loop until matching property has been found
  fdt_node_header_ptr_t node;
  fdt_property_ptr_t prop;
  uint32_t* foo = ( uint32_t* )lex;
  uint32_t len, name_length;
  bool loop = true;
  char* check_str;

  while ( loop ) {
    switch ( be32toh( *foo ) )
    {
      // skip nops
      case FDT_NOP:
        foo++;
        break;

      // check for property is within path
      case FDT_PROP:
        // save property for access
        prop = ( fdt_property_ptr_t )foo;
        // get name and length for comparison
        check_str = ( char* )( string + be32toh( prop->nameoff ) );
        name_length = boot_strlen( check_str );

        // determine length
        len = be32toh( prop->len );
        // add alignment offset
        if ( len % sizeof( uint32_t ) ) {
          len += sizeof( uint32_t ) - len % sizeof( uint32_t );
        }
        // handle empty
        if ( 0 == len ) {
          len += sizeof( uint32_t );
        }

        // consider only if nesting level reached
        if ( NULL == boot_strchr( path, '/' ) ) {
          // check for match
          if ( 0 == boot_strncmp( check_str, path, name_length ) ) {
            if ( ( offset * sizeof( uint32_t ) ) <= be32toh( prop->len ) ) {
              // get address
              uint32_t* addr = ( uint32_t* )prop->data;
              // return address
              return ( uintptr_t )be32toh( *( addr + offset ) );
            }
          }
        }
        // get to next
        foo = ( uint32_t* )( ( uint8_t* )foo + sizeof( fdt_property_t ) + len );
        break;

      case FDT_NODE_BEGIN:
        node = ( fdt_node_header_ptr_t )foo;
        // extract next path
        char *next = boot_strchr( path, '/' );
        // determine length
        len = name_length = boot_strlen( node->name );

        // handle nothing => invalid
        if (
          NULL == next
          && 0 < name_length
          && 0 == boot_strncmp( node->name, path, name_length )
        ) {
          return ( uintptr_t )NULL;
        }

        // add alignment offset
        if ( len % sizeof( uint32_t ) ) {
          len += sizeof( uint32_t ) - len % sizeof( uint32_t );
        }
        // handle empty
        if ( 0 == len ) {
          len += sizeof( uint32_t );
        }
        // add offset to get to next entry if not matching
        foo += 2 + name_length / 4;
        // handle match
        if (
          ( size_t )( next - path ) == name_length
          && 0 == boot_strncmp( node->name, path, name_length )
        ) {
          // recursive call
          return fdt_parse_address_recursive(
            next,
            offset,
            ( uintptr_t )foo,
            string
          );
        }
        break;

      case FDT_NODE_END:
        foo++;
        break;

      default:
        loop = false;
        break;
    }
  }

  return ( uintptr_t )NULL;
}

/**
 * @brief Parse flattened device try for address in property
 *
 * @param path full path to property
 * @param address flattened device tree address
 * @param offset possible offset, when property contains 2 or more addresses
 * @return uintptr_t found address or NULL
 */
uintptr_t __bootstrap fdt_parse_address(
  const char* path,
  uintptr_t address,
  size_t offset
) {
  // get struct and string offset
  uintptr_t lex = address + fdt_header_get_off_dt_struct( address );
  uintptr_t string = address + fdt_header_get_off_dt_strings( address );

  // parse address recursive
  return fdt_parse_address_recursive(
    path,
    offset,
    lex,
    string
  );
}
