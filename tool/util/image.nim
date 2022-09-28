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

from std/os import fileExists, dirExists, createDir, copyDir, copyFile, removeFile, joinPath, getCurrentDir, walkDirRec, pcFile
from system/io import File, open, fmAppend
from std/strutils import intToStr, find, endsWith, replace
from std/math import pow
from std/osproc import execProcess

var mega: int = int( pow( 2.0, 20.0 ) )
var block_size: int = 512;

proc image_dd( target: string, size: int ): void =
  # ensure file is not existing
  if fileExists( target ):
    echo "file " & target & " already exists"
    quit( 1 )
  # transfer MiB to byte
  var byte_size: int = size * mega
  # open file
  var f:File = open( target, fmAppend )
  # write byte per byte to target
  for i in 0 .. byte_size:
    write(f, '0')

proc getDestinationPath( output: string, separator: string, offset: int ): string =
  let position: int = output.find( separator )
  if -1 == position:
    return ""
  var lower: int = position + offset
  var upper: int = output.high
  if output.endsWith( "." ):
    upper -= 1
  return output[ lower..upper ]

proc image_create_partition( target: string, total_size: int, boot_folder: string, boot_type: string, boot_size: int, root_folder: string, root_type: string ): void =
  # use boot size - 1 for mbr
  let partition_size_boot: int = ( boot_size - 1 ) * mega
  # calculate partition root size
  let partition_size_root: int = ( total_size - boot_size ) * mega
  # save partition table offset
  let partition_table_offset: int = mega

  let block_size_image: int = 512
  let block_size: int = 1024
  var current_offset: int = 0

  let base_path: string = joinPath( getCurrentDir(), "tmp" )
  let boot_partition: string = joinPath( base_path, "boot.img" )
  let root_partition: string = joinPath( base_path, "root.img" )

  # create raw files for boot and root
  echo execProcess( "dd if=/dev/zero of=" & boot_partition & " bs=" & intToStr( block_size ) & " count=" & intToStr( int( partition_size_boot / block_size ) ) & " skip=" & intToStr( int( current_offset / block_size ) ) )
  echo execProcess( "dd if=/dev/zero of=" & root_partition & " bs=" & intToStr( block_size ) & " count=" & intToStr( int( partition_size_root / block_size ) ) & " skip=" & intToStr( int( current_offset / block_size ) ) )
  # format raw files ( boot with fat32 and root with ext2 )
  echo execProcess( "mkfs.vfat -F32 " & boot_partition & " --mbr=no" )
  for file in walkDirRec( joinPath( base_path, "partition", "boot" ), { pcFile } ):
    echo execProcess( "mcopy -i " & boot_partition & " " & file & " ::" )
  echo execProcess( """mke2fs -d """" & joinPath( base_path, "partition", "root" ) & """" -r 1 -N 0 -m 5 """" & root_partition & """" """" & intToStr( int( partition_size_root / mega ) ) & """M"""" )
  #echo execProcess( "mkfs.ext2 " & root_partition )
  # FIXME: FIX USER IN GENERATED ext2 IMAGE
  # create raw image file
  echo execProcess( "dd if=/dev/zero of=" & target & " bs=" & intToStr( block_size ) & " count=" & intToStr( int( ( partition_table_offset + partition_size_boot + partition_size_root ) / block_size ) ) & " skip=" & intToStr( int( current_offset / block_size ) ) )

  # set partitions
  let outp_partition_type = execProcess( """printf "
type=0c, size=""" & intToStr( int( partition_size_boot / block_size_image ) ) & """

type=83, size=""" & intToStr( int( partition_size_root / block_size_image ) ) & """
" | sfdisk """" & target & """"""" )
  echo outp_partition_type

  # increment current offset
  current_offset += partition_table_offset
  # copy boot partition
  echo execProcess( "dd if=" & boot_partition & " of=" & target & " bs=" & intToStr( block_size ) & " seek=" & intToStr( int( current_offset / block_size ) ) )
  # increment current offset
  current_offset += partition_size_boot
  # copy root partition
  echo execProcess( "dd if=" & root_partition & " of=" & target & " bs=" & intToStr( block_size ) & " seek=" & intToStr( int( current_offset / block_size ) ) )
  # increment current offset
  current_offset += partition_size_root

proc create_plain_image_file*( image_type: string ): void =
  # bunch of variables
  let base_path: string = joinPath( getCurrentDir(), "tmp" )
  let image_path: string = joinPath( base_path, "image.img" )
  let boot_directory_path: string = joinPath( base_path, "partition", "boot" )
  let root_directory_path: string = joinPath( base_path, "partition", "root" )
  let root_boot_directory_path: string = joinPath( root_directory_path, "boot" )
  let root_etc_directory_path: string = joinPath( root_directory_path, "etc" )
  if "raspi" == image_type:
    # create image file
    echo "Creating plain image with size of 256M"
    image_dd( image_path, 256 )
    # create folder for boot mount point and etc folder for fstab
    createDir( root_boot_directory_path )
    createDir( root_etc_directory_path )
    # copy fstab to partition root etc folder
    copyFile( joinPath( getCurrentDir(), "file", image_type, "fstab" ), joinPath( root_etc_directory_path, "fstab" ) )
    # create boot partition
    echo "Creating boot partition with 100M and root partition with rest"
    image_create_partition( image_path, 256, boot_directory_path, "fat32", 100, root_directory_path, "ext2" )
    let destination_image_path: string = joinPath( getCurrentDir(), "..", "build-aux", "platform", image_type, "sdcard.img" )
    echo "Copying image to build-aux"
    if fileExists( destination_image_path ):
      removeFile( destination_image_path );
    copyFile( image_path, destination_image_path )
