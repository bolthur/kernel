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

#ifndef _KEYMAP_H
#define _KEYMAP_H

#include "../../../../../library/usb/usb.h"

#define KEYMAP_PHY_MAX_CODE 127

#define KEYMAP_NORMTAB 0
#define KEYMAP_SHIFTTAB 1
#define KEYMAP_ALTTAB 2
#define KEYMAP_ALTSHIFTTAB 3

typedef enum
{
  KEYMAP_SPECIAL_KEY_NONE = 0,
  KEYMAP_SPECIAL_KEY_SPACE = 0x100,
  KEYMAP_SPECIAL_KEY_ESCAPE,
  KEYMAP_SPECIAL_KEY_BACKSPACE,
  KEYMAP_SPECIAL_KEY_TABULATOR,
  KEYMAP_SPECIAL_KEY_RETURN,
  KEYMAP_SPECIAL_KEY_INSERT,
  KEYMAP_SPECIAL_KEY_HOME,
  KEYMAP_SPECIAL_KEY_PAGE_UP,
  KEYMAP_SPECIAL_KEY_DELETE,
  KEYMAP_SPECIAL_KEY_END,
  KEYMAP_SPECIAL_KEY_PAGE_DOWN,
  KEYMAP_SPECIAL_KEY_UP,
  KEYMAP_SPECIAL_KEY_DOWN,
  KEYMAP_SPECIAL_KEY_LEFT,
  KEYMAP_SPECIAL_KEY_RIGHT,
  KEYMAP_SPECIAL_KEY_F1,
  KEYMAP_SPECIAL_KEY_F2,
  KEYMAP_SPECIAL_KEY_F3,
  KEYMAP_SPECIAL_KEY_F4,
  KEYMAP_SPECIAL_KEY_F5,
  KEYMAP_SPECIAL_KEY_F6,
  KEYMAP_SPECIAL_KEY_F7,
  KEYMAP_SPECIAL_KEY_F8,
  KEYMAP_SPECIAL_KEY_F9,
  KEYMAP_SPECIAL_KEY_F10,
  KEYMAP_SPECIAL_KEY_F11,
  KEYMAP_SPECIAL_KEY_F12,
  KEYMAP_SPECIAL_KEY_APPLICATION,
  KEYMAP_SPECIAL_KEY_CAPS_LOCK,
  KEYMAP_SPECIAL_KEY_PRINT_SCREEN,
  KEYMAP_SPECIAL_KEY_SCROLL_LOCK,
  KEYMAP_SPECIAL_KEY_PAUSE,
  KEYMAP_SPECIAL_KEY_NUM_LOCK,
  KEYMAP_SPECIAL_KEY_DIVIDE,
  KEYMAP_SPECIAL_KEY_MULTIPLY,
  KEYMAP_SPECIAL_KEY_SUBTRACT,
  KEYMAP_SPECIAL_KEY_ADD,
  KEYMAP_SPECIAL_KEY_ENTER,
  KEYMAP_SPECIAL_KEY_KEYPAD_1,
  KEYMAP_SPECIAL_KEY_KEYPAD_2,
  KEYMAP_SPECIAL_KEY_KEYPAD_3,
  KEYMAP_SPECIAL_KEY_KEYPAD_4,
  KEYMAP_SPECIAL_KEY_KEYPAD_5,
  KEYMAP_SPECIAL_KEY_KEYPAD_6,
  KEYMAP_SPECIAL_KEY_KEYPAD_7,
  KEYMAP_SPECIAL_KEY_KEYPAD_8,
  KEYMAP_SPECIAL_KEY_KEYPAD_9,
  KEYMAP_SPECIAL_KEY_KEYPAD_0,
  KEYMAP_SPECIAL_KEY_KEYPAD_CENTER,
  KEYMAP_SPECIAL_KEY_KEYPAD_COMMA,
  KEYMAP_SPECIAL_KEY_KEYPAD_PERIOD,
  KEYMAP_SPECIAL_KEY_MAX_CODE,
} keymap_special_key_t;

#define KEYPAD_FIRST 0x53
#define KEYPAD_LAST 0x63

typedef struct {
  uint16_t keymap[ KEYMAP_PHY_MAX_CODE + 1 ][ KEYMAP_ALTSHIFTTAB + 1 ];

  bool caps_lock;
  bool num_lock;
  bool scroll_lock;
} keymap_t;

int keymap_init( void );
int keymap_translate( uint16_t, const libusb_keyboard_device_t*, uint16_t* );

#endif
