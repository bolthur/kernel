
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>

#include <panic.h>
#include <arch/arm/atag.h>

void atag_parse( const void *addr ) {
  atag_t *current = ( atag_t* )addr;

  printf( "\r\n" );

  while( current ) {
    switch ( current->tag ) {
      case ATAG_CORE:
        printf("core\r\n");
        break;

      case ATAG_MEMORY:
        printf(
          "memory start: 0x%08x\r\nmemory size: 0x%08x\r\n",
          current->memory.start,
          current->memory.size
        );
        break;

      // unused
      case ATAG_VIDEOTEXT:
        printf("videotext:\r\n");
        break;

      case ATAG_RAMDISK:
        printf("ramdisk:\r\n");
        break;

      case ATAG_INITRD2:
        printf(
          "initrd2 start: 0x%08x\r\ninitrd2 size: 0x%08x\r\n",
          current->initrd2.start,
          current->initrd2.size
        );
        break;

      case ATAG_SERIAL:
        printf("serial:\r\n");
        break;

      case ATAG_REVISION:
        printf("revision:\r\n");
        break;

      case ATAG_VIDEOLFB:
        printf("videolfn\r\n");
        break;

      case ATAG_CMDLINE:
        printf( "cmdline: %s", &current->cmdline.line[ 0 ] );
        break;

      default:
        PANIC( "Unknown tag" );
    }

    // next atag
    current = atag_next( current );
  }
}

bool atag_check( const void* addr ) {
  // access atag at memory
  atag_t *current = ( atag_t* )addr;
  bool valid = false;

  // loop through until first valid atag
  while( current ) {
    // found an atag? => set flag and break loop
    if ( ATAG_NONE != current->tag ) {
      valid = true;
      break;
    }

    // next atag
    current = atag_next( current );
  }

  // return state
  return valid;
}

atag_t *atag_next( const atag_t *current ) {
  if ( ATAG_NONE == current->tag ) {
    return NULL;
  }

  return ( atag_t* )( ( ( uint32_t* )current ) + current->size );
}
