
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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

#include <string.h>
#include <kernel/debug.h>
#include <vendor/rpi/mailbox/mailbox.h>
#include <vendor/rpi/mailbox/property.h>

/**
 * @brief property tag buffer, which needs to be aligned to 16 byte boundary
 */
static int32_t ptb[ 1024 ] ALIGNED( 16 );

/**
 * @brief property tag buffer index used internally
 */
static int32_t ptb_index = 0;

/**
 * @brief Initialize mailbox property process
 */
void mailbox_property_init( void ) {
  // Add startup size
  ptb[ PT_OSIZE ] = 12;

  // process request, everything else seems to be reserved
  ptb[ PT_OREQUEST_OR_RESPONSE ] = 0;

  // first available data slot
  ptb_index = 2;

  // 0-tag to terminate list after init
  ptb[ ptb_index ] = 0;
}

/**
 * @brief Add tag to mailbox property process
 *
 * @param tag Tag to add
 * @param ... Further data depending on tag to be added
 *
 * @todo check and complete
 */
void mailbox_property_add_tag( rpi_mailbox_tag_t tag, ... ) {
  va_list vl;
  va_start( vl, tag );

  // start with adding the tag itself
  ptb[ ptb_index++ ] = tag;

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
      ptb[ ptb_index++ ] = 8;
      ptb[ ptb_index++ ] = 0; // request
      ptb_index += 2;
      break;

    case TAG_GET_CLOCK_STATE:
    case TAG_GET_CLOCK_RATE:
    case TAG_GET_MAX_CLOCK_RATE:
    case TAG_GET_MIN_CLOCK_RATE:
    case TAG_GET_TURBO:
      // provide 8-byte buffer for response
      ptb[ ptb_index++ ] = 8;
      ptb[ ptb_index++ ] = 0; // request
      ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // clock
      ptb_index += 2;
      break;

    case TAG_GET_CLOCKS:
    case TAG_GET_COMMAND_LINE:
      // provide a 256-byte buffer
      ptb[ ptb_index++ ] = 256;
      ptb[ ptb_index++ ] = 0; // request
      ptb_index += 256 >> 2;
      break;

    case TAG_ALLOCATE_BUFFER:
      ptb[ ptb_index++ ] = 8;
      ptb[ ptb_index++ ] = 0; // request
      ptb[ ptb_index++ ] = va_arg( vl, int32_t );
      ptb_index += 1;
      break;

    case TAG_GET_PHYSICAL_SIZE:
    case TAG_SET_PHYSICAL_SIZE:
    case TAG_TEST_PHYSICAL_SIZE:
    case TAG_GET_VIRTUAL_SIZE:
    case TAG_SET_VIRTUAL_SIZE:
    case TAG_TEST_VIRTUAL_SIZE:
    case TAG_GET_VIRTUAL_OFFSET:
    case TAG_SET_VIRTUAL_OFFSET:
      ptb[ ptb_index++ ] = 8;
      ptb[ ptb_index++ ] = 0; // request

      if(
        tag == TAG_SET_PHYSICAL_SIZE
        || tag == TAG_SET_VIRTUAL_SIZE
        || tag == TAG_SET_VIRTUAL_OFFSET
        || tag == TAG_TEST_PHYSICAL_SIZE
        || tag == TAG_TEST_VIRTUAL_SIZE
      ) {
        ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // width
        ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // height
      } else {
        ptb_index += 2;
      }
      break;


    case TAG_GET_ALPHA_MODE:
    case TAG_SET_ALPHA_MODE:
    case TAG_GET_DEPTH:
    case TAG_SET_DEPTH:
    case TAG_GET_PIXEL_ORDER:
    case TAG_SET_PIXEL_ORDER:
    case TAG_GET_PITCH:
      ptb[ ptb_index++ ] = 4;
      ptb[ ptb_index++ ] = 0; // request

      if(
        tag == TAG_SET_DEPTH
        || tag == TAG_SET_PIXEL_ORDER
        || tag == TAG_SET_ALPHA_MODE
      ) {
        // color depth, bits-per-pixel \ pixel order state
        ptb[ ptb_index++ ] = va_arg( vl, int32_t );
      } else {
        ptb_index += 1;
      }
      break;

    case TAG_GET_OVERSCAN:
    case TAG_SET_OVERSCAN:
      ptb[ ptb_index++ ] = 16;
      ptb[ ptb_index++ ] = 0; // request

      if( tag == TAG_SET_OVERSCAN ) {
        ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // top pixels
        ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // bottom pixels
        ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // left pixels
        ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // right pixels
      } else {
        ptb_index += 4;
      }
      break;

    case TAG_SET_CLOCK_RATE:
      ptb[ ptb_index++ ] = 12; // request size
      ptb[ ptb_index++ ] = 8; // response size

      ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // clock
      ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // Hz
      ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // turbo

      ptb_index += 2;
      break;

    case TAG_SET_CLOCK_STATE:
      ptb[ ptb_index++ ] = 8; // request size
      ptb[ ptb_index++ ] = 8; // response size

      // push request parameters
      ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // clock
      ptb[ ptb_index++ ] = va_arg( vl, int32_t ); // state

      ptb_index += 2;
      break;

    default:
      // Unsupported tag, just remove it
      ptb_index--;
      break;
  }

  // Null termination of tag buffer
  ptb[ ptb_index ] = 0;

  va_end( vl );
}

/**
 * @brief Execute mailbox property process
 *
 * @return uint32_t mailbox read result after write
 */
uint32_t mailbox_property_process( void ) {
  uint32_t result;

  // set correct size
  ptb[ PT_OSIZE ] = ( ptb_index + 1 ) << 2;
  ptb[ PT_OREQUEST_OR_RESPONSE ] = 0;

  // debug output
  #if defined( PRINT_MAILBOX )
    DEBUG_OUTPUT( "Length = %d\r\n", ptb[ PT_OSIZE ] );
    for ( int32_t i = 0; i < ( ptb[ PT_OSIZE ] >> 2 ); i++ ) {
      DEBUG_OUTPUT( "Request = %3d %08x\r\n", i, ptb[ i ] );
    }
  #endif

  // write to mailbox
  mailbox_write( MAILBOX0_TAGS_ARM_TO_VC, GPU_MAILBOX, ( uint32_t )ptb );

  // read result
  result = mailbox_read( MAILBOX0_TAGS_ARM_TO_VC, GPU_MAILBOX );

  // debug output
  #if defined( PRINT_MAILBOX )
    for ( int32_t i = 0; i < ( ptb[ PT_OSIZE ] >> 2 ); i++ ) {
      DEBUG_OUTPUT( "Response = %3d %08x\r\n", i, ptb[ i ] );
    }
  #endif

  // return result
  return result;
}

/**
 * @brief Read tag from previous executed mailbox property process
 *
 * @param tag tag to read from mailbox property process
 * @return rpi_mailbox_property_t* Pointer to structure of tag or NULL
 */
rpi_mailbox_property_t* mailbox_property_get( rpi_mailbox_tag_t tag ) {
  // property structure for return and tag buffer
  static rpi_mailbox_property_t property;
  int32_t* tag_buffer = NULL;

  // setup property structure to return
  property.tag = tag;

  // Get the tag from the buffer and start with first available tag position
  int32_t index = 2;

  while( index < ( ptb[ PT_OSIZE ] >> 2 ) ) {
    // debug output
    #if defined( PRINT_MAILBOX )
      DEBUG_OUTPUT( "testing tag[ %d ] = %08x\r\n", index, ptb[ index ] );
    #endif

    // test tag
    if( ptb[ index ] == ( int32_t )tag ) {
      tag_buffer = &ptb[index];
      break;
    }

    // progress with next tag if we haven't yet discovered the wanted tag
    index += ( ptb[index + 1] >> 2 ) + 3;
  }

  // nothing found, return NULL
  if( tag_buffer == NULL ) {
    return NULL;
  }

  // copy necessary data into return structure
  property.byte_length = tag_buffer[ T_ORESPONSE ] & 0xFFFF;
  memcpy(
    property.data.buffer_u8,
    &tag_buffer[ T_OVALUE ],
    ( size_t )property.byte_length
  );

  // return address of static return
  return &property;
}
