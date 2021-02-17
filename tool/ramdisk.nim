#
# Copyright (C) 2018 - 2021 bolthur project.
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

# create tmp directories
createDir( "tmp" )

# check argument count
let argc: int = paramCount()
if argc != 2:
  echo "Usage: ramdisk <dirver path> <initrd name>"
  quit( 1 )

# get command line arguments
let driver: string = paramStr( 1 );
let output: string = paramStr( 2 );

# loop through files of folder including subfolders
for file in walkDirRec( driver ):
  let outp_shell = execProcess( "file " & file )
  if contains( outp_shell, ": ELF" ) and contains( outp_shell, "executable" ):
    # split file path
    let split = splitPath( file )
    let splitted_head = split.head.split( DirSep )
    let executable = split.tail

    # determine target folder
    var target_folder = $DirSep
    var last = len( splitted_head ) - 1
    if executable == splitted_head[ last ]: last = last - 1
    if "core" == splitted_head[ last ] or "driver" == splitted_head[ last ]:
      target_folder = target_folder & splitted_head[ last ] & $DirSep

    # special handling for init
    if executable == "init":
      copyFile( file, joinPath( "tmp", executable ) )
    else:
      let base_path = joinPath( "tmp", "ramdisk", target_folder )
      createDir( base_path )
      copyFile( file, joinPath( base_path, executable ) )

# create ramdisk and remove normal directory
echo execProcess( "tar czvf ../ramdisk.tar.gz *", "tmp/ramdisk" )
removeDir( "tmp/ramdisk" )
# create initrd with init and compressed ramdisk and cleanup directory
echo execProcess( "tar cvf ../" & output & " *", "tmp" )
# remove tmp dir again
removeDir("tmp")
