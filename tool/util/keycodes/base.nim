#
# Copyright (C) 2018 - 2025 bolthur project.
#
# This file is part of bolthur/kernel.
#
# bolthur/kernel is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# bolthur/kernel is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
#

const KeyMapRowCount = 128
const KeyMapColumnCount = 4

type KeyMapRow = array[ KeyMapColumnCount, uint16 ]
type KeyMap* = array[ KeyMapRowCount, KeyMapRow ]

type KeyMapSepcialKey* {.pure.} = enum
  KeyNone = 0,
  KeySpace = 0x100,
  KeyEscape,
  KeyBackspace,
  KeyTabulator,
  KeyReturn,
  KeyInsert,
  KeyHome,
  KeyPageUp,
  KeyDelete,
  KeyEnd,
  KeyPageDown,
  KeyUp,
  KeyDown,
  KeyLeft,
  KeyRight,
  KeyF1,
  KeyF2,
  KeyF3,
  KeyF4,
  KeyF5,
  KeyF6,
  KeyF7,
  KeyF8,
  KeyF9,
  KeyF10,
  KeyF11,
  KeyF12,
  KeyApplication,
  KeyCapsLock,
  KeyPrintScreen,
  KeyScrollLock,
  KeyPause,
  KeyNumLock,
  KeyKP_Divide,
  KeyKP_Multiply,
  KeyKP_Subtract,
  KeyKP_Add,
  KeyKP_Enter,
  KeyKP_1,
  KeyKP_2,
  KeyKP_3,
  KeyKP_4,
  KeyKP_5,
  KeyKP_6,
  KeyKP_7,
  KeyKP_8,
  KeyKP_9,
  KeyKP_0,
  KeyKP_Center,
  KeyKP_Comma,
  KeyKP_Period,
  KeyMaxCode
