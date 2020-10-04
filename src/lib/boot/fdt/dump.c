
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

#include <core/debug/debug.h>
#include <boot/fdt.h>
#include <boot/string.h>
#include <stdio.h>

static int32_t fdt_dump_level;

static uint32_t* fdt_dump_recursive(
  uintptr_t lex,
  uintptr_t string
) {
  fdt_node_header_ptr_t node;
  fdt_property_ptr_t prop;
  uint32_t* foo = ( uint32_t* )lex;
  uint32_t len, name_length;
  char* check_str;
  bool loop = true;

  while ( loop ) {
    switch ( be32toh( *foo ) ) {
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

        // print
        printf( "%*s%s\r\n", fdt_dump_level * 2, "", check_str );
        // get to next
        foo = ( uint32_t* )( ( uint8_t* )foo + sizeof( fdt_property_t ) + len );
        break;

      case FDT_NODE_BEGIN:
        node = ( fdt_node_header_ptr_t )foo;
        // determine length
        len = name_length = boot_strlen( node->name );

        // add alignment offset
        if ( len % sizeof( uint32_t ) ) {
          len += sizeof( uint32_t ) - len % sizeof( uint32_t );
        }
        // handle empty
        if ( 0 == len ) {
          len += sizeof( uint32_t );
        }

        // print name
        printf( "%*s%s{\r\n", fdt_dump_level * 2, "", node->name );

        // add offset to get to next entry if not matching
        foo += 2 + name_length / 4;

        // continue recursive
        fdt_dump_level++;
        foo = fdt_dump_recursive( ( uintptr_t )foo, string );
        break;

      case FDT_NODE_END:
        // reset level
        fdt_dump_level--;
        // print closing tag
        printf( "%*s%c\r\n", fdt_dump_level * 2, "", '}' );
        return ++foo;
        break;

      default:
        loop = false;
    }
  }

  return NULL;
}

void fdt_dump( uintptr_t address ) {
  // get struct and string offset
  uintptr_t lex = address + fdt_header_get_off_dt_struct( address );
  uintptr_t string = address + fdt_header_get_off_dt_strings( address );
  // setup static level for dump
  fdt_dump_level = 0;
  // start recursive dump
  fdt_dump_recursive( lex, string );
}
