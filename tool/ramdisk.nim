#
# Copyright (C) 2018 - 2020 bolthur project.
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

import os
import osproc
import strutils
import re

# create tmp dir
createDir("./tmp")
createDir("./tmp/boot")

# FIXME: ADD DRIVER DIR AS COMMAND LINE PARAMETER
for file in walkDirRec "../build/driver":
  let outp_shell = execProcess("file " & file)
  if contains(outp_shell, ": ELF") and contains(outp_shell, "executable"):
    echo file
    let split = splitPath(file)
    copyFile(file, "./tmp/boot/" & split.tail);

# create tar archive
# FIXME: MOVE NAME TO PARAMETER
discard execProcess("tar cvf ./initrd tmp/**/* --transform='s:tmp/::g'")

# remove tmp dir again
removeDir("./tmp")
