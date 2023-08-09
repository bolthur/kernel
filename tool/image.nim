#
# Copyright (C) 2018 - 2023 bolthur project.
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
from std/osproc import execCmdEx
import std/strutils
# local stuff
from util/scan import scanDirectory
from util/boot import copyFileToBoot, loadFirmwareToBoot
from util/image import createPlainImageFile

# remove tmp dir again
removeDir( "tmp" )
# create tmp directories
createDir( "tmp" )

# FIXME: USE PATCHELF TO REPLACE RPATH OF LIBRARIES AND EXECUTABLES

# check argument count
let argc: int = paramCount()
if argc != 6:
  echo "Usage: ramdisk <root source path> <build path> <sysroot> <bootConfig> <kernel build path> <firmware type>"
  quit( 1 )

# get command line arguments
let rootPath: string = paramStr( 1 )
let buildPath: string = paramStr( 2 )
let sysroot: string = paramStr( 3 )
let bootConfig: string = paramStr( 4 )
let kernelPath: string = paramStr( 5 )
let firmwareType: string = paramStr( 6 )
let outputRamdisk: string = joinPath( rootPath, "build-aux", "platform", firmwareType, "initrd" )

let font: string = joinPath( rootPath, "thirdparty", "font" )
let application: string = joinPath( buildPath, "bolthur", "application" )
let server: string = joinPath( buildPath, "bolthur", "server" )
let bosl: string = joinPath( rootPath, "bosl" )

echo "scanning directories to prepare content of boot, root and ramdisk"
# scan directories and populate image and ramdisk folders
scanDirectory( joinPath( sysroot, "lib" ), "LSB shared object", "", sysroot, false )
scanDirectory( font, "PC Screen Font", "", sysroot, false )
scanDirectory( server, "ELF", "executable", sysroot, false )
scanDirectory( application, "ELF", "executable", sysroot, false )
scanDirectory( bosl, "ASCII", "", sysroot, false )

#[
echo "Fixing up dynamic linker"
# loop through files of folder including subfolders and adjust interpreter and run path for ramdisk
for file in walkDirRec( joinPath( getCurrentDir(), "tmp", "ramdisk" ) ):
  let outpShell = execProcess( "file " & file )
  if contains( outpShell, ": ELF" ) and contains( outpShell, "dynamically linked" ):
    # Replace dynamic linker
    if not contains( outpShell, "LSB shared object" ):
      discard execProcess( "patchelf --set-interpreter /ramdisk/lib/ld-bolthur.so " & file )
    # Replace rpath
    discard execProcess( "patchelf --set-rpath /ramdisk/lib " & file )
]#

# configuration file for ramdisk is handled manually
if dirExists( bootConfig ):
  echo "Copying over boot config to compressed ramdisk"
  # create base path
  let basePath = joinPath( getCurrentDir(), "tmp", "ramdisk", "compressed", "ramdisk", "config" )
  createDir( basePath )
  for file in walkDirRec( bootConfig ):
    # remove full path, and strip possible leading directory separator
    let path = replace( file, bootConfig, "" ).strip( trailing = false, chars = { DirSep } )
    # get folder path out of file
    let splitted = splitPath( path )
    if "" != splitted.head: createDir( joinPath( basePath, splitted.head ) )
    # copy file
    copyFile( file, joinPath( basePath, path ) )

echo "Creating compressed ramdisk"
# create ramdisk and remove normal directory
var cmdResult: tuple[output: string, exitCode: int] = execCmdEx( "tar czvf ../ramdisk.tar.gz * --owner=0 --group=0", workingDir = "tmp/ramdisk/compressed" )
if 0 != cmdResult.exitCode:
  echo "Failed to generate compressed ramdisk: " & cmdResult.output
  quit( 1 )
# cleanup directory
removeDir( "tmp/ramdisk/compressed" )

echo "Creating container around compressed ramdisk and boot process"
# create initrd with init and compressed ramdisk
cmdResult = execCmdEx( "tar cvf " & outputRamdisk & " * --owner=0 --group=0", workingDir = "tmp/ramdisk" )
if 0 != cmdResult.exitCode:
  echo "Failed to generate outer ramdisk container: " & cmdResult.output
  quit( 1 )
# cleanup directory
removeDir( "tmp/ramdisk" )

echo "Loading firmware to image boot folder"
# load necessary firmware and populate into boot
loadFirmwareToBoot( firmwareType )
echo "Copying kernel and ramdisk to image boot folder"
# copy project related stuff to boot partition
copyFileToBoot( kernelPath )
copyFileToBoot( outputRamdisk )
# some dummy directory
createDir( joinPath( getCurrentDir(), "tmp", "partition", "boot", "foobarlongfolder", "foo", "bar" ) )

echo "Creating boot and root images from folders"
# create image
createPlainImageFile( firmwareType, rootPath )

echo "Cleaning up temporary stuff"
# remove tmp dir again
removeDir( "tmp" )
