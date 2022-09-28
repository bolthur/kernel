#
# Copyright (C) 2018 - 2022 bolthur project.
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
import std/osproc
import std/strutils
# local stuff
from util/scan import scan_directory
from util/boot import copy_file_to_boot, load_firmware_to_boot
from util/image import create_plain_image_file

# remove tmp dir again
removeDir( "tmp" )
# create tmp directories
createDir( "tmp" )

# FIXME: USE PATCHELF TO REPLACE RPATH OF LIBRARIES AND EXECUTABLES

# check argument count
let argc: int = paramCount()
if argc != 9:
  echo "Usage: ramdisk <sysroot> <root path> <output ramdisk> <boot_config> <kernel path> <kernel config> <kernel cmdline> <firmware type> <output_image>"
  quit( 1 )

# get command line arguments
let sysroot: string = paramStr( 1 )
let root_path: string = paramStr( 2 )
let output_ramdisk: string = paramStr( 3 )
let boot_config: string = paramStr( 4 )
let kernel_path: string = paramStr( 5 )
let kernel_config: string = paramStr( 6 )
let kernel_cmdline: string = paramStr( 7 )
let firmware_type: string = paramStr( 8 )
let output_image: string = paramStr( 9 )

let font: string = joinPath( root_path, "..", "thirdparty", "font" )
let application: string = joinPath( root_path, "bolthur", "application" )
let server: string = joinPath( root_path, "bolthur", "server" )
let bosl: string = joinPath( root_path, "..", "bosl" )

# scan directories and populate image and ramdisk folders
scan_directory( joinPath( sysroot, "lib" ), "LSB shared object", "", sysroot, false )
scan_directory( font, "PC Screen Font", "", sysroot, false )
scan_directory( server, "ELF", "executable", sysroot, false )
scan_directory( application, "ELF", "executable", sysroot, false )
scan_directory( bosl, "ASCII", "", sysroot, false )

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

# configuration file for ramdisk is handled manually
if dirExists( boot_config ):
  # create base path
  let base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", "compressed", "ramdisk", "config" )
  createDir( base_path )
  for file in walkDirRec( boot_config ):
    # remove full path, and strip possible leading directory separator
    let path = replace( file, boot_config, "" ).strip( trailing = false, chars = { DirSep } )
    # get folder path out of file
    let splitted = splitPath( path )
    if "" != splitted.head: createDir( joinPath( base_path, splitted.head ) )
    # copy file
    copyFile( file, joinPath( base_path, path ) )

# create ramdisk and remove normal directory
echo execProcess( "tar czvf ../ramdisk.tar.gz * --owner=0 --group=0", "tmp/ramdisk/compressed" )
removeDir( "tmp/ramdisk/compressed" )
# create initrd with init and compressed ramdisk and cleanup directory
if output_ramdisk.isAbsolute:
  echo execProcess( "tar cvf " & output_ramdisk & " * --owner=0 --group=0", "tmp/ramdisk" )
else:
  echo execProcess( "tar cvf ../" & output_ramdisk & " * --owner=0 --group=0 --exclude='./partition'", "tmp/ramdisk" )
removeDir( "tmp/ramdisk" )

# load necessary firmware and populate into boot
load_firmware_to_boot( firmware_type )
# copy project related stuff to boot partition
copy_file_to_boot( kernel_path )
copy_file_to_boot( kernel_config )
copy_file_to_boot( kernel_cmdline )
copy_file_to_boot( output_ramdisk )

# create image
create_plain_image_file( firmware_type )

# FIXME: CREATE ROOT AND BOOT PARTITIONS
# remove tmp dir again
removeDir( "tmp" )
