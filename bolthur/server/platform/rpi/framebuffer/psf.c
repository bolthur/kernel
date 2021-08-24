/**
 * Copyright (C) 2018 - 2021 bolthur project.
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "psf.h"

uint8_t* font_buffer = NULL;
size_t font_buffer_size = 0;
psf_font_ptr_t font = NULL;

/**
 * @fn bool psf_load_font(void)
 * @brief Helper to load pc screen font
 *
 * @return
 */
static bool psf_load_font( void ) {
  // open executable
  int fd = open( "/ramdisk/font/zap-vga09.psf", O_RDONLY );
  // check file descriptor return
  if ( -1 == fd ) {
    return false;
  }
  // get to end of file
  long position = lseek( fd, 0, SEEK_END );
  if ( -1 == position ) {
    close( fd );
    return false;
  }
  font_buffer_size = ( size_t )position;
  // reset back to beginning
  if ( -1 == lseek( fd, 0, SEEK_SET ) ) {
    close( fd );
    return false;
  }
  // allocate management space
  font_buffer = malloc( font_buffer_size * sizeof( uint8_t ) );
  if ( ! font_buffer ) {
    close( fd );
    return false;
  }
  // read whole file
  ssize_t n = read( fd, font_buffer, font_buffer_size );
  // handle error
  if ( -1 == n ) {
    free( font_buffer );
    close( fd );
    return false;
  }
  // handle possible error
  if ( ( size_t )n != font_buffer_size ) {
    free( font_buffer );
    close( fd );
    return false;
  }
  // close opened file handle again
  close( fd );
  return true;
}

/**
 * @fn bool psf_init(void)
 * @brief Initialize all necessary
 *
 * @return
 */
bool psf_init( void ) {
  // load font to memory
  if ( ! psf_load_font() ) {
    return false;
  }
  // allocate management structure
  font = malloc( sizeof( *font ) );
  if( ! font ) {
    return false;
  }

  // minimum size needed for both versions
  if ( 4 > font_buffer_size ) {
    return false;
  }
  // check size for v1
  if (
    PSF1_MAGIC0 == font_buffer[ 0 ]
    && PSF1_MAGIC1 == font_buffer[ 1 ]
    && sizeof( psf_font_header_v1_t ) > font_buffer_size
  ) {
    EARLY_STARTUP_PRINT( "Invalid font size for v1!\r\n" )
    return false;
  }
  // check size for v2
  if (
    PSF1_MAGIC0 != font_buffer[ 0 ]
    && PSF1_MAGIC1 != font_buffer[ 1 ]
    && sizeof( psf_font_header_v2_t ) > font_buffer_size
  ) {
    EARLY_STARTUP_PRINT( "Invalid font size for v2!\r\n" )
    return false;
  }

  // set correct type in management structure
  if (
    PSF1_MAGIC0 == font_buffer[ 0 ]
    && PSF1_MAGIC1 == font_buffer[ 1 ]
  ) {
    font->type = PSF_FONT_HEADER_TYPE_V1;
  } else if (
    PSF2_MAGIC0 == font_buffer[ 0 ]
    && PSF2_MAGIC1 == font_buffer[ 1 ]
    && PSF2_MAGIC2 == font_buffer[ 2 ]
    && PSF2_MAGIC3 == font_buffer[ 3 ]
  ) {
    font->type= PSF_FONT_HEADER_TYPE_V2;
  } else {
    EARLY_STARTUP_PRINT( "Invalid magic %#"PRIx16", %#"PRIx32"!\r\n",
      *( ( uint16_t* )font_buffer ),
      *( ( uint32_t* )font_buffer )
    )
    return false;
  }

  // copy over header information
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    memcpy( &font->header.v1, font_buffer, sizeof( psf_font_header_v1_t ) );
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    memcpy( &font->header.v2, font_buffer, sizeof( psf_font_header_v2_t ) );
  }

  // return success
  return true;
}

/**
 * @fn uint32_t psf_glyph_size(void)
 * @brief returns single glyph size
 *
 * @return
 */
uint32_t psf_glyph_size( void ) {
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    return ( uint32_t )font->header.v1.height;
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    return font->header.v2.charsize;
  }
  return 0;
}
/**
 * @fn uint32_t psf_glyph_height(void)
 * @brief returns height of a single glyph
 *
 * @return
 */
uint32_t psf_glyph_height( void ) {
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    return ( uint32_t )font->header.v1.height;
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    return font->header.v2.height;
  }
  return 0;
}

/**
 * @fn uint32_t psf_glyph_width(void)
 * @brief returns width of a single glyph
 *
 * @return
 */
uint32_t psf_glyph_width( void ) {
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    return 8;
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    return font->header.v2.width;
  }
  return 0;
}

/**
 * @fn uint32_t psf_glyph_total(void)
 * @brief Returns total glyph count
 *
 * @return
 */
uint32_t psf_glyph_total( void ) {
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    if ( font->header.v1.mode & PSF1_MODE512 ) {
      return 512;
    } else {
      return 256;
    }
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    return font->header.v2.length;
  }
  return 0;
}

/**
 * @fn uint8_t psf_char_to_glyph*(uint8_t)
 * @brief Return glyph representing character
 *
 * @param c
 * @return
 */
uint8_t* psf_char_to_glyph( uint8_t c ) {
  // determine header size of font
  size_t header_size;
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    header_size = sizeof( font->header.v1 );
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    header_size = sizeof( font->header.v2 );
  } else {
    return NULL;
  }

  // determine glyph
  uint32_t off = 0;
  if ( c > 0 && c < psf_glyph_total() ) {
    off = c * psf_glyph_size();
  }
  return ( uint8_t* )( font_buffer + header_size + off );
}
