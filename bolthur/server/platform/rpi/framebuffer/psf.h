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

#include <stdint.h>
#include <stdbool.h>

#if ! defined( __PSF__ )
#define __PSF__

// v1 magic
#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04
// v1 mode bits
#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE 0x05
// v1 separator and sequence start
#define PSF1_SEPARATOR 0xFFFF
#define PSF1_STARTSEQ 0xFFFE

// v2 magic
#define PSF2_MAGIC0 0x72
#define PSF2_MAGIC1 0xb5
#define PSF2_MAGIC2 0x4a
#define PSF2_MAGIC3 0x86
// v2 flags
#define PSF2_HAS_UNICODE_TABLE 0x01
// v2 max version
#define PSF2_MAXVERSION 0
// v2 separator and sequence start
#define PSF2_SEPARATOR 0xFF
#define PSF2_STARTSEQ 0xFE

struct psf_font_header_v1 {
  uint8_t magic[ 2 ];
  uint8_t mode;
  uint8_t height;
};
typedef struct psf_font_header_v1 psf_font_header_v1_t;
typedef struct psf_font_header_v1* psf_font_header_v1_ptr_t;

struct psf_font_header_v2{
  uint8_t magic8[ 4 ];
  uint32_t version;
  uint32_t header_size;
  uint32_t flags;
  uint32_t length;
  uint32_t charsize;
  uint32_t height;
  uint32_t width;
};
typedef struct psf_font_header_v2 psf_font_header_v2_t;
typedef struct psf_font_header_v2* psf_font_header_v2_ptr_t;

enum psf_font_header_type {
  PSF_FONT_HEADER_TYPE_V1 = 0,
  PSF_FONT_HEADER_TYPE_V2,
};
typedef enum psf_font_header_type psf_font_header_type_t;

struct psf_font {
  psf_font_header_type_t type;
  union {
    psf_font_header_v1_t v1;
    psf_font_header_v2_t v2;
  } header;
};
typedef struct psf_font psf_font_t;
typedef struct psf_font* psf_font_ptr_t;

bool psf_init( void );
uint32_t psf_glyph_size( void );
uint32_t psf_glyph_height( void );
uint32_t psf_glyph_width( void );
uint32_t psf_glyph_total( void );
uint8_t* psf_char_to_glyph( uint8_t );

#endif
