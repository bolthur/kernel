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
# thirdparty
import zippy

# remove tmp dir again
removeDir( "tmp" )
# create tmp directories
createDir( "tmp" )

# FIXME: USE PATCHELF TO REPLACE RPATH OF LIBRARIES AND EXECUTABLES

# check argument count
let argc: int = paramCount()
if argc != 3:
  echo "Usage: ramdisk <driver path> <initrd name> <sysroot>"
  quit( 1 )

# get command line arguments
let driver: string = paramStr( 1 );
let output: string = paramStr( 2 );
let sysroot: string = paramStr( 3 );

# loop through libraries and iterate over them
for file in walkDirRec( joinPath( sysroot, "lib" ), { pcFile, pcLinkToFile } ):
  var file_to_check = file
  var info = getFileInfo( file, false )
  var symlink_src = ""
  if pcLinkToFile == info.kind:
    # get info with follow symlink again
    info = getFileInfo( file )
    # skip non files
    if pcFile != info.kind:
      continue
    var splitted = file.splitFile()
    # expand symlink and use for check
    file_to_check = expandSymlink( file )
    if not file_to_check.isAbsolute:
      file_to_check = joinPath( splitted.dir, file_to_check )
    # determine symlink source and strip variable
    symlink_src = strip( replace( file_to_check, splitted.dir & DirSep, "" ) )
    # loop as long as file to check is still a symlink
    info = getFileInfo( file_to_check, false )
    while pcLinkToFile == info.kind:
      # expand symlink again
      var splitted = file_to_check.splitFile()
      file_to_check = expandSymlink( file_to_check )
      # add absolute path
      if not file_to_check.isAbsolute:
        file_to_check = joinPath( splitted.dir, file_to_check )
      # get info again
      info = getFileInfo( file_to_check, false )
  let outp_shell = execProcess( "file " & file_to_check )
  if contains( outp_shell, "LSB shared object" ):
    # split file path
    let split = splitPath( file )
    #let splitted_head = split.head.split( DirSep )
    let library = split.tail
    # copy library
    let base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", "ramdisk", "usr", "lib" )
    createDir( base_path )
    if not isEmptyOrWhitespace( symlink_src ):
      createSymlink( symlink_src, joinPath( base_path, library ) )
    else:
      # Add shared object compressed
      #writeFile( joinPath( base_path, library ), zippy.compress( readFile( file ), zippy.DefaultCompression, zippy.dfGzip ) )
      copyFile( file, joinPath( base_path, library ) )

# loop through true type fonts and iterate over them
for file in walkDirRec( joinPath( sysroot, "share", "font2" ), { pcFile, pcLinkToFile } ):
  var file_to_check = file
  var info = getFileInfo( file, false )
  var symlink_src = ""
  if pcLinkToFile == info.kind:
    # get info with follow symlink again
    info = getFileInfo( file )
    # skip non files
    if pcFile != info.kind:
      continue
    var splitted = file.splitFile()
    # expand symlink and use for check
    file_to_check = expandSymlink( file )
    if not file_to_check.isAbsolute:
      file_to_check = joinPath( splitted.dir, file_to_check )
    # determine symlink source and strip variable
    symlink_src = strip( replace( file_to_check, splitted.dir & DirSep, "" ) )
    # loop as long as file to check is still a symlink
    info = getFileInfo( file_to_check, false )
    while pcLinkToFile == info.kind:
      # expand symlink again
      var splitted = file_to_check.splitFile()
      file_to_check = expandSymlink( file_to_check )
      # add absolute path
      if not file_to_check.isAbsolute:
        file_to_check = joinPath( splitted.dir, file_to_check )
      # get info again
      info = getFileInfo( file_to_check, false )
  let outp_shell = execProcess( "file " & file_to_check )
  if contains( outp_shell, "TrueType Font" ):
    # split file path
    let split = splitPath( file )
    #let splitted_head = split.head.split( DirSep )
    let font = split.tail
    # copy library
    let base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", "ramdisk", "share", "font" )
    createDir( base_path )
    if not isEmptyOrWhitespace( symlink_src ):
      createSymlink( symlink_src, joinPath( base_path, font ) )
    else:
      # Add font compressed
      writeFile( joinPath( base_path, font ), zippy.compress( readFile( file ), zippy.DefaultCompression, zippy.dfGzip ) )
      #copyFile( file, joinPath( base_path, font ) )

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

    var skip_top_ramdisk = false
    var pos = -1
    for idx, value in splitted_head:
      if value == "driver" or value == "core" or value == "usr":
        pos = idx
    if not ( "src" in splitted_head[ pos..^1 ] ) and pos != -1:
      target_folder &= splitted_head[ pos..^2 ].join( $DirSep )
    else:
      pos = -1

    # special handling for init
    if executable == "init" and -1 == pos:
      copyFile( file, joinPath( getCurrentDir(), "tmp", executable ) )
    else:
      var base_path = ""
      if skip_top_ramdisk:
        base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", target_folder )
      else:
        base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", "ramdisk", target_folder )
      createDir( base_path )
      copyFile( file, joinPath( base_path, executable ) )

#[
# loop through files of folder including subfolders and adjust interpreter and run path for ramdisk
for file in walkDirRec( joinPath( getCurrentDir(), "tmp", "ramdisk" ) ):
  let outp_shell = execProcess( "file " & file )
  if contains( outp_shell, ": ELF" ) and contains( outp_shell, "dynamically linked" ):
    # Replace dynamic linker
    if not contains( outp_shell, "LSB shared object" ):
      discard execProcess( "patchelf --set-interpreter /ramdisk/lib/ld-bolthur.so " & file )
    # Replace rpath
    discard execProcess( "patchelf --set-rpath /ramdisk/lib " & file )
]#

# append testing stuff from current dir
for kind, file in walkDir(getCurrentDir()):
  # split file
  let split = splitFile( file )
  let file_name = split.name
  let ending = split.ext
  if ending == ".txt":
    let base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", "ramdisk", "test" )
    createDir( base_path )
    copyFile( file, joinPath( base_path, file_name & ending ) )

# create ramdisk and remove normal directory
echo execProcess( "tar czvf ../ramdisk.tar.gz * --owner=0 --group=0", "tmp/ramdisk" )
removeDir( "tmp/ramdisk" )
# create initrd with init and compressed ramdisk and cleanup directory
if output.isAbsolute:
  echo execProcess( "tar cvf " & output & " * --owner=0 --group=0", "tmp" )
else:
  echo execProcess( "tar cvf ../" & output & " * --owner=0 --group=0", "tmp" )
# remove tmp dir again
removeDir( "tmp" )
