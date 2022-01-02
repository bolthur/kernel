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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#if defined( PRINT_MAILBOX )
  #include <debug/debug.h>
#endif
#include <entry.h>
#include <mm/phys.h>
#include <platform/rpi/mailbox/mailbox.h>
#include <platform/rpi/mailbox/property.h>

/**
 * @brief property tag buffer index used internally
 */
int32_t ptb_index = 0;

/**
 * @brief property tag buffer
 */
int32_t* ptb_buffer = NULL;

/**
 * @brief phys property tag buffer
 */
volatile int32_t* ptb_buffer_phys = NULL;

/**
 * @fn void mailbox_property_init(void)
 * @brief Initialize mailbox property process
 */
void mailbox_property_init( void ) {
  // reserve memory if not yet done
  if ( ! ptb_buffer ) {
    ptb_buffer = ( int32_t* )aligned_alloc( PAGE_SIZE, PAGE_SIZE );
    assert( ptb_buffer )
    ptb_buffer_phys = ( int32_t* )VIRT_2_PHYS( ptb_buffer );
  }
  // clear out buffer
  memset( ptb_buffer, 0, PAGE_SIZE );
  // Add startup size
  ptb_buffer[ PT_OSIZE ] = 12;
  // process request, everything else seems to be reserved
  ptb_buffer[ PT_OREQUEST_OR_RESPONSE ] = 0;
  // first available data slot
  ptb_index = 2;
  // 0-tag to terminate list after init
  ptb_buffer[ ptb_index ] = 0;
}

/**
 * @fn void mailbox_property_add_tag(rpi_mailbox_tag_t, ...)
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
  ptb_buffer[ ptb_index++ ] = tag;

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
      ptb_buffer[ ptb_index++ ] = 8;
      // request
      ptb_buffer[ ptb_index++ ] = 0;
      ptb_index += 2;
      break;

    case TAG_GET_CLOCK_STATE:
    case TAG_GET_CLOCK_RATE:
    case TAG_GET_MAX_CLOCK_RATE:
    case TAG_GET_MIN_CLOCK_RATE:
    case TAG_GET_TURBO:
      // provide 8-byte buffer for response
      ptb_buffer[ ptb_index++ ] = 8;
      // request
      ptb_buffer[ ptb_index++ ] = 0;
      // clock
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // increase index
      ptb_index += 2;
      break;

    case TAG_GET_CLOCKS:
    case TAG_GET_COMMAND_LINE:
      // provide a 256-byte buffer
      ptb_buffer[ ptb_index++ ] = 256;
      ptb_buffer[ ptb_index++ ] = 0;
      // increase index
      ptb_index += 256 >> 2;
      break;

    case TAG_ALLOCATE_BUFFER:
      ptb_buffer[ ptb_index++ ] = 8;
      // request
      ptb_buffer[ ptb_index++ ] = 0;
      // pass argument
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // increase index
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
      ptb_buffer[ ptb_index++ ] = 8;
      // request
      ptb_buffer[ ptb_index++ ] = 0;

      if (
        tag == TAG_SET_PHYSICAL_SIZE
        || tag == TAG_SET_VIRTUAL_SIZE
        || tag == TAG_SET_VIRTUAL_OFFSET
        || tag == TAG_TEST_PHYSICAL_SIZE
        || tag == TAG_TEST_VIRTUAL_SIZE
      ) {
        // width
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
        // height
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
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
      ptb_buffer[ ptb_index++ ] = 4;
      // request
      ptb_buffer[ ptb_index++ ] = 0;

      if (
        tag == TAG_SET_DEPTH
        || tag == TAG_SET_PIXEL_ORDER
        || tag == TAG_SET_ALPHA_MODE
      ) {
        // color depth, bits-per-pixel \ pixel order state
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      } else {
        ptb_index += 1;
      }
      break;

    case TAG_GET_OVERSCAN:
    case TAG_SET_OVERSCAN:
      ptb_buffer[ ptb_index++ ] = 16;
      // request
      ptb_buffer[ ptb_index++ ] = 0;

      if ( tag == TAG_SET_OVERSCAN ) {
        // top pixels
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
        // bottom pixels
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
        // left pixels
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
        // right pixels
        ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      } else {
        ptb_index += 4;
      }
      break;

    case TAG_SET_CLOCK_RATE:
      // request size
      ptb_buffer[ ptb_index++ ] = 12;
      // response size
      ptb_buffer[ ptb_index++ ] = 8;

      // clock
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // hz
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // turbo
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // increase index for return
      ptb_index += 2;
      break;

    case TAG_SET_CLOCK_STATE:
      // request size
      ptb_buffer[ ptb_index++ ] = 8;
      // response size
      ptb_buffer[ ptb_index++ ] = 8;
      // push request parameters
      // clock
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // state
      ptb_buffer[ ptb_index++ ] = va_arg( vl, int32_t );
      // increase index for return
      ptb_index += 2;
      break;

    default:
      // Unsupported tag, just remove it
      ptb_index--;
      break;
  }

  // Null termination of tag buffer
  ptb_buffer[ ptb_index ] = 0;

  va_end( vl );
}

/**
 * @fn uint32_t mailbox_property_process(void)
 * @brief Execute mailbox property process
 *
 * @return_t mailbox read result after write
 */
uint32_t mailbox_property_process( void ) {
  uint32_t result;

  // set correct size
  ptb_buffer[ PT_OSIZE ] = ( ptb_index + 1 ) << 2;
  ptb_buffer[ PT_OREQUEST_OR_RESPONSE ] = 0;

  // debug output
  #if defined( PRINT_MAILBOX )
    DEBUG_OUTPUT( "Length = %d\r\n", ptb_buffer[ PT_OSIZE ] );
    for ( int32_t i = 0; i < ( ptb_buffer[ PT_OSIZE ] >> 2 ); i++ ) {
      DEBUG_OUTPUT( "Request = %3d %08x\r\n", i, ptb_buffer[ i ] );
    }
  #endif

  // write to mailbox
  mailbox_write(
    MAILBOX0_TAGS_ARM_TO_VC,
    GPU_MAILBOX,
    ( uint32_t )ptb_buffer_phys );

  // read result
  result = mailbox_read( MAILBOX0_TAGS_ARM_TO_VC, GPU_MAILBOX );

  // debug output
  #if defined( PRINT_MAILBOX )
    for ( int32_t i = 0; i < ( ptb_buffer[ PT_OSIZE ] >> 2 ); i++ ) {
      DEBUG_OUTPUT( "Response = %3d %08x\r\n", i, ptb_buffer[ i ] );
    }
  #endif

  // return result
  return result;
}

/**
 * @fn rpi_mailbox_property_t mailbox_property_get*(rpi_mailbox_tag_t)
 * @brief Read tag from previous executed mailbox property process
 *
 * @param tag tag to read from mailbox property process
 * @return pointer to structure of tag or NULL
 */
rpi_mailbox_property_t* mailbox_property_get( rpi_mailbox_tag_t tag ) {
  // property structure for return and tag buffer
  static rpi_mailbox_property_t property;
  int32_t* tag_buffer = NULL;

  // Get the tag from the buffer and start with first available tag position
  int32_t index = 2;
  int32_t size = ptb_buffer[ PT_OSIZE ] >> 2;

  while ( index < size ) {
    // debug output
    #if defined( PRINT_MAILBOX )
      DEBUG_OUTPUT( "testing tag[ %d ] = %08x\r\n", index, ptb_buffer[ index ] );
    #endif

    // test tag
    if ( ptb_buffer[ index ] == ( int32_t )tag ) {
      tag_buffer = &ptb_buffer[ index ];
      break;
    }

    // progress with next tag if we haven't yet discovered the wanted tag
    index += ( ptb_buffer[ index + 1 ] >> 2 ) + 3;
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
    ( size_t )property.byte_length );

  // return address of static return
  return &property;
}
