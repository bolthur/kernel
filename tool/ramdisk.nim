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

# create tmp dir
discard existsOrCreateDir("tmp")
discard existsOrCreateDir("tmp/ramdisk")
discard existsOrCreateDir("tmp/ramdisk/driver")

# check argument count
let argc: int = paramCount()
if argc != 2:
  echo "Usage: ramdisk <dirver path> <initrd name>"
  quit(1)

# get command line arguments
let driver: string = paramStr( 1 );
let output: string = paramStr( 2 );

for file in walkDirRec driver:
  let outp_shell = execProcess("file " & file)
  if contains(outp_shell, ": ELF") and contains(outp_shell, "executable"):
    echo file
    let split = splitPath(file)
    if split.tail == "init":
      copyFile(file, "tmp/" & split.tail);
    else:
      copyFile(file, "tmp/ramdisk/driver/" & split.tail);

# create ramdisk
echo execProcess("tar czvf ../ramdisk.tar.gz *", "tmp/ramdisk")
removeDir("tmp/ramdisk")
# create initrd with init and compressed ramdisk
echo execProcess("tar cvf ../" & output & " *", "tmp")

# remove tmp dir again
removeDir("tmp")
