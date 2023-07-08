/**
 * Copyright (C) 2018 - 2023 bolthur project.
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
#include <endian.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "psf.h"

// FIXME: ADD VALUE CONVERSION FROM ENDIAN HEADER OVERALL

psf_font_t* font = NULL;

/**
 * @fn bool psf_load_font(psf_font_t*)
 * @brief Load font and push to passed structure
 *
 * @param f
 * @return
 *
 * @todo move path to font into parameter
 */
static bool psf_load_font( psf_font_t* f ) {
  // open executable
  int fd = open( "/ramdisk/font/zap-vga09.psf", O_RDONLY );
  // check file descriptor return
  if ( -1 == fd ) {
    return false;
  }
  // get to end of file
  off_t position = lseek( fd, 0, SEEK_END );
  if ( -1 == position ) {
    close( fd );
    return false;
  }
  f->font_buffer_size = ( size_t )position;
  // reset back to beginning
  if ( -1 == lseek( fd, 0, SEEK_SET ) ) {
    close( fd );
    return false;
  }
  // allocate management space
  f->font_buffer = malloc( f->font_buffer_size * sizeof( uint8_t ) );
  if ( ! f->font_buffer ) {
    close( fd );
    return false;
  }
  // read whole file
  ssize_t n = read( fd, f->font_buffer, f->font_buffer_size );
  // handle error
  if ( -1 == n ) {
    free( f->font_buffer );
    close( fd );
    return false;
  }
  // handle possible error
  if ( ( size_t )n != f->font_buffer_size ) {
    free( f->font_buffer );
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
 *
 * @todo add parse of unicode table for v2
 */
bool psf_init( void ) {
  // allocate management structure
  font = malloc( sizeof( *font ) );
  if ( ! font ) {
    return false;
  }
  // load font to memory
  if ( ! psf_load_font( font ) ) {
    return false;
  }

  // minimum size needed for both versions
  if ( 4 > font->font_buffer_size ) {
    return false;
  }
  // check size for v1
  if (
    PSF1_MAGIC0 == font->font_buffer[ 0 ]
    && PSF1_MAGIC1 == font->font_buffer[ 1 ]
    && sizeof( psf_font_header_v1_t ) > font->font_buffer_size
  ) {
    free( font->font_buffer );
    free( font );
    return false;
  }
  // check size for v2
  if (
    PSF1_MAGIC0 != font->font_buffer[ 0 ]
    && PSF1_MAGIC1 != font->font_buffer[ 1 ]
    && sizeof( psf_font_header_v2_t ) > font->font_buffer_size
  ) {
    free( font->font_buffer );
    free( font );
    return false;
  }

  // set correct type in management structure
  if (
    PSF1_MAGIC0 == font->font_buffer[ 0 ]
    && PSF1_MAGIC1 == font->font_buffer[ 1 ]
  ) {
    font->type = PSF_FONT_HEADER_TYPE_V1;
  } else if (
    PSF2_MAGIC0 == font->font_buffer[ 0 ]
    && PSF2_MAGIC1 == font->font_buffer[ 1 ]
    && PSF2_MAGIC2 == font->font_buffer[ 2 ]
    && PSF2_MAGIC3 == font->font_buffer[ 3 ]
  ) {
    font->type = PSF_FONT_HEADER_TYPE_V2;
  } else {
    free( font->font_buffer );
    free( font );
    return false;
  }

  // copy over header information
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    memcpy( &font->header.v1, font->font_buffer, sizeof( psf_font_header_v1_t ) );
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    memcpy( &font->header.v2, font->font_buffer, sizeof( psf_font_header_v2_t ) );
  } else {
    free( font->font_buffer );
    free( font );
    return false;
  }

  // handle unicode offset
  uint32_t unicode_offset = psf_unicode_table_offset();
  if (
    PSF_FONT_HEADER_TYPE_V1 == font->type
    && 0 < unicode_offset
  ) {
    // get unicode table and calculate end of buffer
    uint16_t* table = ( uint16_t* )( font->font_buffer + unicode_offset );
    uint16_t* end = ( uint16_t* )( font->font_buffer + font->font_buffer_size );
    uint16_t glyph = 0;
    // allocate unicode mapping table
    font->unicode = calloc( USHRT_MAX, 2 );
    if ( ! font->unicode ) {
      free( font->font_buffer );
      free( font );
      return false;
    }
    // loop until end has been reached
    while ( table < end ) {
      // fetch unicode for mapping
      uint16_t uc = *table;
      // handle next glyph
      if ( PSF1_SEPARATOR == uc ) {
        glyph++;
        table++;
        continue;
      }
      // skip start sequence
      if ( PSF1_STARTSEQ == uc ) {
        table++;
        continue;
      }
      // add mapping
      font->unicode[ uc ] = glyph;
      table++;
    }
  } else if (
    PSF_FONT_HEADER_TYPE_V2 == font->type
    && 0 < unicode_offset
  ) {
    // FIXME: ADD SUPPORT!
    free( font->font_buffer );
    free( font );
    return false;
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
 * @fn uint32_t psf_unicode_table_offset(void)
 * @brief Get unicode offset table if existing
 *
 * @return
 */
uint32_t psf_unicode_table_offset( void ) {
  // determine header size of font
  size_t header_size;
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    header_size = sizeof( font->header.v1 );
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    header_size = sizeof( font->header.v2 );
  } else {
    return 0;
  }
  // return whole offset
  if (
    (
      PSF_FONT_HEADER_TYPE_V1 == font->type
      && ( font->header.v1.mode & PSF1_MODEHASTAB )
    ) || (
      PSF_FONT_HEADER_TYPE_V2 == font->type
      && ( font->header.v2.flags & PSF2_HAS_UNICODE_TABLE )
    )
  ) {
    return header_size + psf_glyph_total() * psf_glyph_size();
  }
  return 0;
}

/**
 * @fn uint8_t psf_char_to_glyph*(uint32_t)
 * @brief Return glyph representing character
 *
 * @param c
 * @return
 */
uint8_t* psf_char_to_glyph( uint32_t c ) {
  // determine header size of font
  size_t header_size;
  if ( PSF_FONT_HEADER_TYPE_V1 == font->type ) {
    header_size = sizeof( font->header.v1 );
  } else if ( PSF_FONT_HEADER_TYPE_V2 == font->type ) {
    header_size = sizeof( font->header.v2 );
  } else {
    return NULL;
  }

  // overwrite glyph if mapping exists
  if ( font->unicode && font->unicode[ c ] ) {
    c = font->unicode[ c ];
  }
  if ( ! c ) {
    return NULL;
  }

  // determine glyph
  uint32_t off = 0;
  if ( c < psf_glyph_total() ) {
    off += c * psf_glyph_size();
  }
  return ( uint8_t* )( font->font_buffer + header_size + off );
}
