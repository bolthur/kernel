/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

// system includes
#include <ft2build.h>
#include FT_FREETYPE_H
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/bolthur.h>
// local includes
#include "ttf.h"
#include "global.h"

static bool initialized = false;
static FT_Library library;
static FT_Face face;
static size_t font_buffer_size = 0;
static uint8_t* font_buffer = nullptr;

/**
 * @fn bool load_font_into_memory(const char*)
 * @brief Helper to load font into memory
 * @param filename
 * @return
 */
static bool load_font_into_memory( const char* filename ) {
  // open executable
  const int fd = open( filename, O_RDONLY );
  // check file descriptor return
  if ( -1 == fd ) {
    return false;
  }
  // get to end of file
  const off_t position = lseek( fd, 0, SEEK_END );
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
  const ssize_t n = read( fd, font_buffer, font_buffer_size );
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
 * @fn bool ttf_init(const char*, uint32_t, uint32_t)
 * @brief Initialize ttf
 * @return
 */
bool ttf_init( const char* filename, const uint32_t width, const uint32_t height ) {
  // destroy if initialized
  if ( initialized ) {
    #if defined( TERMINAL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Destroying ttf\r\n" )
    #endif
    ttf_destroy();
  }
  #if defined( TERMINAL_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Init free type\r\n" )
  #endif
  // init freetype library
  if ( FT_Init_FreeType( &library ) ) {
    #if defined( TERMINAL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to init free type library\r\n" )
    #endif
    return false;
  }
  #if defined( TERMINAL_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Loading font into buffer\r\n" )
  #endif
  if ( ! load_font_into_memory( filename ) ) {
    #if defined( TERMINAL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed loading font into buffer\r\n" )
    #endif
    return false;
  }
  #if defined( TERMINAL_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Loading font face %s\r\n", filename )
  #endif
  // load font face
  if ( FT_New_Memory_Face( library, font_buffer, ( FT_Long )font_buffer_size, 0, &face ) ) {
    #if defined( TERMINAL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to load font %s\r\n", filename )
    #endif
    FT_Done_FreeType( library );
    return false;
  }
  #if defined( TERMINAL_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Set pixel sizes\r\n" )
  #endif
  // set font size
  if ( FT_Set_Pixel_Sizes( face, width, height ) ) {
    #if defined( TERMINAL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to set font size\r\n" )
    #endif
    FT_Done_Face( face );
    FT_Done_FreeType( library );
    return false;
  }
  #if defined( TERMINAL_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Done setup ttf\r\n" )
  #endif
  // set initialized
  initialized = true;
  // return success
  return true;
}

/**
 * @fn void ttf_destroy(void)
 * @brief Destroy ttf
 */
void ttf_destroy( void ) {
  if ( initialized ) {
    FT_Done_Face( face );
    FT_Done_FreeType( library );
    if ( font_buffer ) {
      free( font_buffer );
      font_buffer_size = 0;
    }
    initialized = false;
  }
}

/**
 * @fn bool ttf_initialized(void)
 * @brief initialized state for ttf
 * @return
 */
bool ttf_initialized( void ) {
  return initialized;
}

/**
 * @fn uint32_t ttf_glyph_height(void)
 * @brief Function to get set glyph height in pixel
 * @return
 */
uint32_t ttf_glyph_height( void ) {
  return ( uint32_t )( face->size->metrics.height >> 6 );
}

/**
 * @fn uint32_t ttf_glyph_height(void)
 * @brief Function to get set glyph height in pixel
 * @return
 */
uint32_t ttf_glyph_width( void ) {
  return ( uint32_t )( face->size->metrics.max_advance >> 6 );
}

/**
 * @fn void ttf_render_char(uint8_t*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
 * @brief Helper to render character to passed surface
 *
 * @param surface
 * @param depth
 * @param pitch
 * @param c
 * @param cursor_x
 * @param cursor_y
 * @param color_fg
 * @param color_bg
 */
void ttf_render_char(
  volatile uint8_t* surface,
  const uint32_t depth,
  const uint32_t pitch,
  const uint32_t c,
  const uint32_t cursor_x,
  const uint32_t cursor_y,
  const uint32_t color_fg,
  const uint32_t color_bg
) {
  // get glyph index
  auto const glyph = FT_Get_Char_Index( face, c );
  // get glyph
  if ( FT_Load_Glyph( face, glyph, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT ) ) {
    #if defined( TERMINAL_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Failed to load glyph\r\n" )
    #endif
    return;
  }
  // cache slot and bitmap
  auto const slot = face->glyph;
  auto const bitmap = &slot->bitmap;
  // get cell width and height
  const int32_t cell_height = ( int32_t )ttf_glyph_height();
  const int32_t cell_width = ( int32_t )ttf_glyph_width();
  const int32_t global_ascender = face->size->metrics.ascender >> 6;
  // calculate cell start x and y
  const int32_t cell_start_y = ( int32_t )( cursor_y * ( uint32_t )cell_height );
  const int32_t cell_start_x = ( int32_t )( cursor_x * ( uint32_t )cell_width );
  // calculate glyph start x and y
  const int32_t glyph_start_y = ( cell_start_y + global_ascender ) - slot->bitmap_top;
  const int32_t glyph_start_x = cell_start_x + slot->bitmap_left;
  // determine bytes per pixel
  const uint32_t bytes_per_pixel = depth / 8;
  // Determine color channels for foreground
  const uint8_t fg_r = ( color_fg >> 16 ) & 0xFF;
  const uint8_t fg_g = ( color_fg >> 8 ) & 0xFF;
  const uint8_t fg_b = color_fg & 0xFF;
  // Determine color channels for background
  const uint8_t bg_r = ( color_bg >> 16 ) & 0xFF;
  const uint8_t bg_g = ( color_bg >> 8 ) & 0xFF;
  const uint8_t bg_b = color_bg & 0xFF;
  // clear with background color
  for ( int32_t row = 0; row < cell_height; row++ ) {
    const int32_t current_y = cell_start_y + row;
    if ( 0 > current_y ) {
      continue;
    }
    volatile uint8_t* target_row_ptr = surface + ( ( uint32_t )current_y * pitch );
    for ( int32_t column = 0; column < cell_width; column++ ) {
      const int32_t current_x = cell_start_x + column;
      if ( 0 > current_x ) {
        continue;
      }
      auto const pixel_addr = ( volatile uint32_t* )( target_row_ptr + ( (uint32_t)current_x * bytes_per_pixel ) );
      *pixel_addr = color_bg;
    }
  }
  // handle nothing to render
  if ( ! bitmap->rows || ! bitmap->width || ! bitmap->buffer ) {
    return;
  }
  // loop through rows
  for ( uint32_t row = 0; row < bitmap->rows; row++ ) {
    // determine current y
    const int32_t current_y = glyph_start_y + ( int32_t )row;
    // handle invalid
    if ( 0 > current_y ) {
      continue;
    }
    // get target row
    volatile uint8_t* target_row_ptr = surface + ( ( uint32_t )current_y * pitch );
    // loop through columns
    for ( uint32_t column = 0; column < bitmap->width; column++ ) {
      // get current x
      const int32_t current_x = glyph_start_x + ( int32_t )column;
      // handle invalid
      if ( 0 > current_x ) {
        continue;
      }
      // Calculate the specific pixel address using pure byte math offsets
      auto const pixel_addr = ( volatile uint32_t* )( target_row_ptr + ( ( uint32_t )current_x * bytes_per_pixel ) );
      // get possible alpha
      const uint8_t alpha = bitmap->buffer[ row * ( uint32_t )bitmap->pitch + column ];
      // background color previously set => skip
      if ( 0 == alpha ) {
        continue;
      }
      if ( 255 == alpha ) {
        *pixel_addr = ( ( uint32_t )fg_r << 16 ) | ( ( uint32_t )fg_g << 8 ) | ( uint32_t )fg_b;
      } else {
        // anti-aliasing: Linear interpolation (Blending) between FG and BG
        constexpr uint8_t max_alpha = 255;
        // calculate red
        const uint32_t blend_r = ( ( uint32_t )fg_r * alpha ) + ( ( uint32_t )bg_r * ( max_alpha - alpha ) );
        const uint8_t res_r = ( uint8_t )( ( blend_r + 1 + ( blend_r >> 8) ) >> 8 );
        // calculate green
        const uint32_t blend_g = ( ( uint32_t )fg_g * alpha ) + ( ( uint32_t )bg_g * ( max_alpha - alpha ) );
        const uint8_t res_g = ( uint8_t )( ( blend_g + 1 + ( blend_g >> 8) ) >> 8 );
        // calculate blue
        const uint32_t blend_b = ( ( uint32_t )fg_b * alpha ) + ( ( uint32_t )bg_b * ( max_alpha - alpha ) );
        const uint8_t res_b = ( uint8_t )( ( blend_b + 1 + ( blend_b >> 8) ) >> 8 );
        // set pixel address
        *pixel_addr = ( ( uint32_t )res_r << 16 ) | ( ( uint32_t )res_g << 8 ) | ( uint32_t )res_b;
      }
    }
  }
}
