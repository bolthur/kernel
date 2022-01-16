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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/bolthur.h>
#include <sys/mman.h>
#include "property.h"
#include "mailbox.h"
#include "generic.h"

#include <inttypes.h>
#include <errno.h>

/**
 * @brief property tag buffer index used internally
 */
int32_t property_index = 0;

/**
 * @brief property tag buffer
 */
int32_t* property_buffer = NULL;

/**
 * @brief phys property tag buffer
 */
volatile int32_t* property_buffer_phys = NULL;

/**
 * @fn bool property_setup(void)
 * @brief Setup property stuff
 *
 * @return
 */
bool property_setup( void ) {
  // map buffer as device memory
  void* tmp_buffer = mmap(
    NULL,
    PAGE_SIZE,
    PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_DEVICE,
    -1,
    0
  );
  if ( MAP_FAILED == tmp_buffer ) {
    return false;
  }
  // push it to property buffer
  property_buffer = ( int32_t* )tmp_buffer;
  // get physical address
  uintptr_t translated = _memory_translate_physical( ( uintptr_t )property_buffer );
  if ( errno ) {
    return false;
  }
  // push physical address to volatile pointer
  property_buffer_phys = ( volatile int32_t* )translated;
  // return success when both are set
  return true;
}

/**
 * @fn void property_init(void)
 * @brief Initialize mailbox property process
 */
void property_init( void ) {
  // clear out buffer
  memset( property_buffer, 0, PAGE_SIZE );
  // Add startup size
  property_buffer[ PT_OSIZE ] = 12;
  // process request, everything else seems to be reserved
  property_buffer[ PT_OREQUEST_OR_RESPONSE ] = 0;
  // first available data slot
  property_index = 2;
  // 0-tag to terminate list after init
  property_buffer[ property_index ] = 0;
}

/**
 * @fn void property_add_tag(raspi_mailbox_tag_t, ...)
 * @brief Add tag to mailbox property process
 *
 * @param tag Tag to add
 * @param ... Further data depending on tag to be added
 *
 * @todo check and complete
 */
void property_add_tag( raspi_mailbox_tag_t tag, ... ) {
  va_list vl;
  va_start( vl, tag );

  // start with adding the tag itself
  property_buffer[ property_index++ ] = tag;

  switch( tag ) {
    case TAG_GET_FIRMWARE_VERSION:
    case TAG_GET_BOARD_MODEL:
    case TAG_GET_BOARD_REVISION:
    case TAG_GET_BOARD_MAC_ADDRESS:
    case TAG_GET_BOARD_SERIAL:
    case TAG_GET_ARM_MEMORY:
    case TAG_GET_VC_MEMORY:
    case TAG_GET_DMA_CHANNELS:
      // provide 8-byte buffer for response
      property_buffer[ property_index++ ] = 8;
      // request
      property_buffer[ property_index++ ] = 0;
      property_index += 2;
      break;

    case TAG_GET_CLOCK_STATE:
    case TAG_GET_CLOCK_RATE:
    case TAG_GET_MAX_CLOCK_RATE:
    case TAG_GET_MIN_CLOCK_RATE:
    case TAG_GET_TURBO:
      // provide 8-byte buffer for response
      property_buffer[ property_index++ ] = 8;
      // request
      property_buffer[ property_index++ ] = 0;
      // clock
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // increase index
      property_index += 2;
      break;

    case TAG_GET_CLOCKS:
    case TAG_GET_COMMAND_LINE:
      // provide a 256-byte buffer
      property_buffer[ property_index++ ] = 256;
      property_buffer[ property_index++ ] = 0;
      // increase index
      property_index += 256 >> 2;
      break;

    case TAG_ALLOCATE_BUFFER:
      property_buffer[ property_index++ ] = 8;
      // request
      property_buffer[ property_index++ ] = 0;
      // pass argument
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // increase index
      property_index += 1;
      break;

    case TAG_GET_PHYSICAL_SIZE:
    case TAG_SET_PHYSICAL_SIZE:
    case TAG_TEST_PHYSICAL_SIZE:
    case TAG_GET_VIRTUAL_SIZE:
    case TAG_SET_VIRTUAL_SIZE:
    case TAG_TEST_VIRTUAL_SIZE:
    case TAG_GET_VIRTUAL_OFFSET:
    case TAG_SET_VIRTUAL_OFFSET:
      property_buffer[ property_index++ ] = 8;
      // request
      property_buffer[ property_index++ ] = 0;

      if (
        tag == TAG_SET_PHYSICAL_SIZE
        || tag == TAG_SET_VIRTUAL_SIZE
        || tag == TAG_SET_VIRTUAL_OFFSET
        || tag == TAG_TEST_PHYSICAL_SIZE
        || tag == TAG_TEST_VIRTUAL_SIZE
      ) {
        // width
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
        // height
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      } else {
        property_index += 2;
      }
      break;

    case TAG_GET_ALPHA_MODE:
    case TAG_SET_ALPHA_MODE:
    case TAG_GET_DEPTH:
    case TAG_SET_DEPTH:
    case TAG_GET_PIXEL_ORDER:
    case TAG_SET_PIXEL_ORDER:
    case TAG_GET_PITCH:
      property_buffer[ property_index++ ] = 4;
      // request
      property_buffer[ property_index++ ] = 0;

      if (
        tag == TAG_SET_DEPTH
        || tag == TAG_SET_PIXEL_ORDER
        || tag == TAG_SET_ALPHA_MODE
      ) {
        // color depth, bits-per-pixel \ pixel order state
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      } else {
        property_index += 1;
      }
      break;

    case TAG_GET_OVERSCAN:
    case TAG_SET_OVERSCAN:
      property_buffer[ property_index++ ] = 16;
      // request
      property_buffer[ property_index++ ] = 0;

      if ( tag == TAG_SET_OVERSCAN ) {
        // top pixels
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
        // bottom pixels
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
        // left pixels
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
        // right pixels
        property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      } else {
        property_index += 4;
      }
      break;

    case TAG_SET_CLOCK_RATE:
      // request size
      property_buffer[ property_index++ ] = 12;
      // response size
      property_buffer[ property_index++ ] = 8;

      // clock
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // hz
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // turbo
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // increase index for return
      property_index += 2;
      break;

    case TAG_SET_CLOCK_STATE:
      // request size
      property_buffer[ property_index++ ] = 8;
      // response size
      property_buffer[ property_index++ ] = 8;
      // push request parameters
      // clock
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // state
      property_buffer[ property_index++ ] = va_arg( vl, int32_t );
      // increase index for return
      property_index += 2;
      break;

    default:
      // Unsupported tag, just remove it
      property_index--;
      break;
  }

  // Null termination of tag buffer
  property_buffer[ property_index ] = 0;

  va_end( vl );
}

/**
 * @fn uint32_t property_process(void)
 * @brief Execute mailbox property process
 *
 * @return_t mailbox read result after write
 */
uint32_t property_process( void ) {
  uint32_t result;
  // set correct size
  property_buffer[ PT_OSIZE ] = ( property_index + 1 ) << 2;
  property_buffer[ PT_OREQUEST_OR_RESPONSE ] = 0;
  // write to mailbox
  mailbox_write(
    MAILBOX0_TAGS_ARM_TO_VC,
    ( uint32_t )property_buffer_phys
  );
  // read and return result
  result = mailbox_read( MAILBOX0_TAGS_ARM_TO_VC );
  return result;
}

/**
 * @fn raspi_property_t property_get*(raspi_mailbox_tag_t)
 * @brief Read tag from previous executed mailbox property process
 *
 * @param tag tag to read from mailbox property process
 * @return pointer to structure of tag or NULL
 */
raspi_property_t* property_get( raspi_mailbox_tag_t tag ) {
  // property structure for return and tag buffer
  static raspi_property_t property;
  int32_t* tag_buffer = NULL;
  // Get the tag from the buffer and start with first available tag position
  int32_t index = 2;
  int32_t size = property_buffer[ PT_OSIZE ] >> 2;
  while ( index < size ) {
    // test tag
    if ( property_buffer[ index ] == ( int32_t )tag ) {
      tag_buffer = &property_buffer[ index ];
      break;
    }
    // progress with next tag if we haven't yet discovered the wanted tag
    index += ( property_buffer[ index + 1 ] >> 2 ) + 3;
  }
  // nothing found, return NULL
  if ( ! tag_buffer ) {
    return NULL;
  }
  // clear return
  memset( &property, 0, sizeof( property ) );
  // setup property structure to return
  property.tag = tag;
  // set byte length
  property.byte_length = tag_buffer[ T_ORESPONSE ] & 0xFFFF;
  // copy necessary data into return structure
  memcpy(
    property.data.buffer_u8,
    &tag_buffer[ T_OVALUE ],
    ( size_t )property.byte_length
  );
  // return address of static return
  return &property;
}
