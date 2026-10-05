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

# Edit file only with iso 8859-15 encoding

from ./base import KeyMap, KeyMapSepcialKey

proc getAndFillKeyMapUs*(): KeyMap =
  return [
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x00
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x01
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x02
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x03
    [ uint16('a'), uint16('A'), uint16('á'), uint16('Á'), ], # 0x04
    [ uint16('b'), uint16('B'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x05
    [ uint16('c'), uint16('C'), uint16('©'), uint16('¢'), ], # 0x06
    [ uint16('d'), uint16('D'), uint16('ð'), uint16('Ð'), ], # 0x07
    [ uint16('e'), uint16('E'), uint16('é'), uint16('É'), ], # 0x08
    [ uint16('f'), uint16('F'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x09
    [ uint16('g'), uint16('G'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x0A
    [ uint16('h'), uint16('H'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x0B
    [ uint16('i'), uint16('I'), uint16('í'), uint16('Í'), ], # 0x0C
    [ uint16('j'), uint16('J'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x0D
    [ uint16('k'), uint16('K'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x0E
    [ uint16('l'), uint16('L'), uint16('ø'), uint16('Ø'), ], # 0x0F
    [ uint16('m'), uint16('M'), uint16('µ'), uint16(ord(KeyNone)), ], # 0x10
    [ uint16('n'), uint16('N'), uint16('ñ'), uint16('Ñ'), ], # 0x11
    [ uint16('o'), uint16('O'), uint16('ó'), uint16('Ó'), ], # 0x12
    [ uint16('p'), uint16('P'), uint16('ö'), uint16('Ö'), ], # 0x13
    [ uint16('q'), uint16('Q'), uint16('ä'), uint16('Ä'), ], # 0x14
    [ uint16('r'), uint16('R'), uint16('®'), uint16(ord(KeyNone)), ], # 0x15
    [ uint16('s'), uint16('S'), uint16('ß'), uint16('§'), ], # 0x16
    [ uint16('t'), uint16('T'), uint16('þ'), uint16('Þ'), ], # 0x17
    [ uint16('u'), uint16('U'), uint16('ú'), uint16('Ú'), ], # 0x18
    [ uint16('v'), uint16('V'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x19
    [ uint16('w'), uint16('W'), uint16('å'), uint16('Å'), ], # 0x1A
    [ uint16('x'), uint16('X'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x1B
    [ uint16('y'), uint16('Y'), uint16('ü'), uint16('Ü'), ], # 0x1C
    [ uint16('z'), uint16('Z'), uint16('æ'), uint16('Æ'), ], # 0x1D
    [ uint16('1'), uint16('!'), uint16('¡'), uint16('¹'), ], # 0x1E
    [ uint16('2'), uint16('@'), uint16('²'), uint16(ord(KeyNone)), ], # 0x1F
    [ uint16('3'), uint16('#'), uint16('³'), uint16(ord(KeyNone)), ], # 0x20
    [ uint16('4'), uint16('$'), uint16('?'), uint16('£'), ], # 0x21
    [ uint16('5'), uint16('%'), uint16('€'), uint16(ord(KeyNone)), ], # 0x22
    [ uint16('6'), uint16('^'), uint16('?'), uint16(ord(KeyNone)), ], # 0x23
    [ uint16('7'), uint16('&'), uint16('?'), uint16(ord(KeyNone)), ], # 0x24
    [ uint16('8'), uint16('*'), uint16('?'), uint16(ord(KeyNone)), ], # 0x25
    [ uint16('9'), uint16('('), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x26
    [ uint16('0'), uint16(')'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x27
    [ uint16(ord(KeyReturn)), uint16(ord(KeyReturn)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x28
    [ uint16(ord(KeyEscape)), uint16(ord(KeyEscape)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x29
    [ uint16(ord(KeyBackspace)), uint16(ord(KeyBackspace)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x2A
    [ uint16(ord(KeyTabulator)), uint16(ord(KeyTabulator)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x2B
    [ uint16(ord(KeySpace)), uint16(ord(KeySpace)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x2C
    [ uint16('-'), uint16('_'), uint16('¥'), uint16(ord(KeyNone)), ], # 0x2D
    [ uint16('='), uint16('+'), uint16('×'), uint16('÷'), ], # 0x2E
    [ uint16('['), uint16('['), uint16('«'), uint16(ord(KeyNone)), ], # 0x2F
    [ uint16(']'), uint16('}'), uint16('»'), uint16(ord(KeyNone)), ], # 0x30
    [ uint16('\\'), uint16('|'), uint16('¬'), uint16('?'), ], # 0x31
    [ uint16('#'), uint16('~'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x32
    [ uint16(';'), uint16(':'), uint16('¶'), uint16('°'), ], # 0x33
    [ uint16('\''), uint16('\"'), uint16('?'), uint16('?'), ], # 0x34
    [ uint16('`'), uint16('~'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x35
    [ uint16(','), uint16('<'), uint16('ç'), uint16('Ç'), ], # 0x36
    [ uint16('.'), uint16('>'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x37
    [ uint16('/'), uint16('?'), uint16('¿'), uint16(ord(KeyNone)), ], # 0x38
    [ uint16(ord(KeyCapsLock)), uint16(ord(KeyCapsLock)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x39
    [ uint16(ord(KeyF1)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x3A
    [ uint16(ord(KeyF2)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x3B
    [ uint16(ord(KeyF3)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x3C
    [ uint16(ord(KeyF4)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x3D
    [ uint16(ord(KeyF5)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x3E
    [ uint16(ord(KeyF6)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x3F
    [ uint16(ord(KeyF7)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x40
    [ uint16(ord(KeyF8)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x41
    [ uint16(ord(KeyF9)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x42
    [ uint16(ord(KeyF10)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x43
    [ uint16(ord(KeyF11)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x44
    [ uint16(ord(KeyF12)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x45
    [ uint16(ord(KeyPrintScreen)),uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x46
    [ uint16(ord(KeyScrollLock)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x47
    [ uint16(ord(KeyPause)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x48
    [ uint16(ord(KeyInsert)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x49
    [ uint16(ord(KeyHome)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x4A
    [ uint16(ord(KeyPageUp)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x4B
    [ uint16(ord(KeyDelete)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x4C
    [ uint16(ord(KeyEnd)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x4D
    [ uint16(ord(KeyPageDown)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x4E
    [ uint16(ord(KeyRight)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x4F
    [ uint16(ord(KeyLeft)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x50
    [ uint16(ord(KeyDown)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x51
    [ uint16(ord(KeyUp)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x52
    [ uint16(ord(KeyNumLock)), uint16(ord(KeyNumLock)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x53
    [ uint16(ord(KeyKP_Divide)), uint16(ord(KeyKP_Divide)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x54
    [ uint16(ord(KeyKP_Multiply)), uint16(ord(KeyKP_Multiply)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x55
    [ uint16(ord(KeyKP_Subtract)), uint16(ord(KeyKP_Subtract)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x56
    [ uint16(ord(KeyKP_Add)), uint16(ord(KeyKP_Add)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x57
    [ uint16(ord(KeyKP_Enter)), uint16(ord(KeyKP_Enter)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x58
    [ uint16(ord(KeyEnd)), uint16(ord(KeyKP_1)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x59
    [ uint16(ord(KeyDown)), uint16(ord(KeyKP_2)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x5A
    [ uint16(ord(KeyPageDown)), uint16(ord(KeyKP_3)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x5B
    [ uint16(ord(KeyLeft)), uint16(ord(KeyKP_4)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x5C
    [ uint16(ord(KeyKP_Center)), uint16(ord(KeyKP_5)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x5D
    [ uint16(ord(KeyRight)), uint16(ord(KeyKP_6)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x5E
    [ uint16(ord(KeyHome)), uint16(ord(KeyKP_7)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x5F
    [ uint16(ord(KeyUp)), uint16(ord(KeyKP_8)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x60
    [ uint16(ord(KeyPageUp)), uint16(ord(KeyKP_9)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x61
    [ uint16(ord(KeyInsert)), uint16(ord(KeyKP_0)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x62
    [ uint16(ord(KeyDelete)), uint16(ord(KeyKP_Period)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x63
    [ uint16('\\'), uint16('|'), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x64
    [ uint16(ord(KeyApplication)), uint16(ord(KeyApplication)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x65
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x66
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x67
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x68
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x69
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x6A
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x6B
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x6C
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x6D
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x6E
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x6F
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x70
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x71
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x72
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x73
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x74
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x75
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x76
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x77
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x78
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x79
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x7A
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x7B
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x7C
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x7D
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x7E
    [ uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), uint16(ord(KeyNone)), ], # 0x7F
  ]
