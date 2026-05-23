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

#include <errno.h>
#include <sys/bolthur.h>
#include <confini.h>
#include "keymap.h"

/**
 * @brief Flag indicating whether keymap is loaded or not
 */
static bool keymap_loaded = false;

/**
 * @brief Keymap populated during runtime
 */
static keymap_t map;

/**
 * @fn int confini_callback(IniDispatch*, void*)
 * @brief Confini load callback
 * @param dispatch
 * @param other
 * @return
 */
static int confini_callback(
  IniDispatch* dispatch,
  [[maybe_unused]] void* other
) {
  const char* name = dispatch->data;
  const char* value = dispatch->value;
  // handle keymap
  if ( 0 == strcmp( name, "KEYMAP" ) ) {
    // debug output
    STARTUP_PRINT( "Allocate space for path for fopen\r\n" )
    // allocate space for path
    char* path = malloc( sizeof( char ) * PATH_MAX );
    // handle allocation error
    if ( ! path ) {
      STARTUP_PRINT( "Unable to allocate path\r\n" )
      return 1;
    }
    // clear out
    memset( path, 0, sizeof( char ) * PATH_MAX );
    // build path to keymap
    snprintf( path, PATH_MAX, "/usr/share/kbd/%s.dat", value );
    // debug output
    STARTUP_PRINT( "Opening %s\r\n", path )
    // open keymap
    FILE* f = fopen( path, "rb" );
    // handle error
    if ( ! f ) {
      // debug output
      STARTUP_PRINT( "Unable to open %s\r\n", path )
      // free path
      free( path );
      // return error
      return 1;
    }
    // debug output
    STARTUP_PRINT( "Reading binary data into keymap array\r\n" )
    // load binary into array
    for ( size_t i = 0; i < KEYMAP_PHY_MAX_CODE + 1; ++i ) {
      // read into keymap
      const size_t read = fread(
        &map.keymap[ i ],
        sizeof( uint16_t ),
        KEYMAP_ALTSHIFTTAB + 1,
        f
      );
      // check read amount
      if ( read != KEYMAP_ALTSHIFTTAB + 1 ) {
        // debug output
        STARTUP_PRINT( "Unable to read keymap entry %zu\r\n", i )
        // close file
        fclose( f );
        // free path
        free( path );
        // return error
        return 1;
      }
    }
    // close file again
    fclose( f );
    // free path again
    free( path );
    // set loaded flag
    keymap_loaded = true;
  }
  // return success
  return 0;
}

/**
 * @fn int keymap_init(void)
 * @brief Function to initialize keymap
 * @return
 */
int keymap_init( void ) {
  // handle loaded
  if ( keymap_loaded ) {
    return 0;
  }
  // clear out map
  memset( &map, 0, sizeof( map ) );
  // open console configuration
  FILE* vconsole = fopen( "/etc/vconsole.conf", "r" );
  // handle error
  if ( ! vconsole ) {
    const int e = errno;
    STARTUP_PRINT( "Unable to open /etc/vconsole.conf: %s\r\n", strerror( e ) );
    return e;
  }
  // parse ini
  if ( load_ini_file(
    vconsole,
    INI_DEFAULT_FORMAT,
    NULL,
    confini_callback,
    NULL
  ) ) {
    EARLY_STARTUP_PRINT( "Cannot load console configuration file!\r\n" )
    return EIO;
  }
  // close ini file again
  fclose( vconsole );
  // handle not loaded
  if ( ! keymap_loaded ) {
    return EINVAL;
  }
  // return success
  return 0;
}

/**
 * @fn int keymap_translate(uint16_t, const libusb_keyboard_device_t*, uint16_t*)
 * @brief Translate physical key code
 * @param physical_code
 * @param dev
 * @param output
 * @return
 */
int keymap_translate( const uint16_t physical_code, const libusb_keyboard_device_t* dev, uint16_t* output ) {
  // validate loaded and output
  if ( ! keymap_loaded || ! output ) {
    return EINVAL;
  }
  // handle no translation
  if ( physical_code > KEYMAP_PHY_MAX_CODE ) {
    *output = KEYMAP_SPECIAL_KEY_NONE;
    return 0;
  }
  // determine table to be used
  uint8_t table = KEYMAP_NORMTAB;
  if ( KEYPAD_FIRST <= physical_code && physical_code <= KEYPAD_LAST ) {
    if ( map.num_lock ) {
      table = KEYMAP_SHIFTTAB;
    }
  } else if ( dev->modifier.right_alt ) {
    if ( dev->modifier.left_shift || dev->modifier.right_shift ) {
      table = KEYMAP_ALTSHIFTTAB;
    } else {
      table = KEYMAP_ALTTAB;
    }
  } else if ( dev->modifier.left_shift || dev->modifier.right_shift ) {
    table = KEYMAP_SHIFTTAB;
  }
  // get key code from keymap and store it in output
  *output = map.keymap[ physical_code ][ table ];
  // return success
  return 0;
}
