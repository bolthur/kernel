#
# Copyright (C) 2018 - 2026 bolthur project.
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

import std/os
import std/streams

# import keymap type
from keycodes/base import KeyMap
# import keycode generators
from keycodes/de import getAndFillKeyMapDe
from keycodes/us import getAndFillKeyMapUs

proc generateKeycodesForImage*( imageType: string ): void =
  # get destination directory
  let destinationDirectory: string = joinPath( getCurrentDir(), "file", imageType, "root", "usr", "share", "kbd" )
  # de
  let deCodes:KeyMap = getAndFillKeyMapDe()
  var f = newFileStream( joinPath( destinationDirectory, "de.dat" ), fmWrite )
  if not isNil(f):
    f.write( deCodes )
    f.flush()
    f.close()
  # us
  let usCodes:KeyMap = getAndFillKeyMapUs()
  f = newFileStream( joinPath( destinationDirectory, "us.dat" ), fmWrite )
  if not isNil(f):
    f.write( usCodes )
    f.flush()
    f.close()
