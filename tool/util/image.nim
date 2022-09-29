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
from std/strutils import intToStr, find, endsWith, replace
from std/math import pow
from std/osproc import execCmdEx

var mega: int = int( pow( 2.0, 20.0 ) )

proc image_dd( source: string, target: string, size: int, block_size: int, offset: int ): void =
  var cmd:string = "dd if=" & source & " of=" & target & " bs=" & intToStr( block_size ) & " count=" & intToStr( int( size / block_size ) ) & " skip=" & intToStr( int( offset / block_size ) )
  if -1 == size:
    cmd = "dd if=" & source & " of=" & target & " bs=" & intToStr( block_size ) & " seek=" & intToStr( int( offset / block_size ) )

  let result: tuple[output: string, exitCode: int] = execCmdEx( cmd )
  if 0 != result.exitCode:
    echo "Unable to create raw image: " & result.output
    quit( 1 )

proc image_create_partition( target: string, total_size: int, boot_folder: string, boot_type: string, boot_size: int, root_folder: string, root_type: string ): void =
  # use boot size - 1 for mbr
  let partition_size_boot: int = ( boot_size - 1 ) * mega
  # calculate partition root size
  let partition_size_root: int = ( total_size - boot_size ) * mega
  # save partition table offset
  let partition_table_offset: int = mega

  var cmd_result: tuple[output: string, exitCode: int];

  let block_size_image: int = 512
  let block_size: int = 1024
  var current_offset: int = 0

  let base_path: string = joinPath( getCurrentDir(), "tmp" )
  let boot_partition: string = joinPath( base_path, "boot.img" )
  let root_partition: string = joinPath( base_path, "root.img" )

  # create raw files for boot, root and destination image
  image_dd( "/dev/zero", boot_partition, partition_size_boot, block_size, current_offset )
  image_dd( "/dev/zero", root_partition, partition_size_root, block_size, current_offset )
  image_dd( "/dev/zero", target, partition_table_offset + partition_size_boot + partition_size_root, block_size, current_offset )
  # format boot image with fat 32
  cmd_result = execCmdEx( "mkfs.vfat -F32 " & boot_partition & " --mbr=no" )
  if 0 != cmd_result.exitCode:
    echo "Unable to format boot partition with FAT32: " & cmd_result.output
    quit( 1 )
  # format root image with ext2
  cmd_result = execCmdEx( """mke2fs -t ext2 -d """" & joinPath( base_path, "partition", "root" ) & """" -r 1 -N 0 -m 5 """" & root_partition & """" """" & intToStr( int( partition_size_root / mega ) ) & """M"""" )
  if 0 != cmd_result.exitCode:
    echo "Unable to format root partition with ext2: " & cmd_result.output
    quit( 1 )
  # copy files to boot partition
  for file in walkDirRec( joinPath( base_path, "partition", "boot" ), { pcFile } ):
    cmd_result = execCmdEx( "mcopy -i " & boot_partition & " " & file & " ::" )
    if 0 != cmd_result.exitCode:
      echo "Unable to copy content to boot partition: " & cmd_result.output
      quit( 1 )
  # FIXME: FIX USER IN GENERATED ext2 IMAGE
  # set partitions
  cmd_result = execCmdEx( """printf "
type=0c, size=""" & intToStr( int( partition_size_boot / block_size_image ) ) & """

type=83, size=""" & intToStr( int( partition_size_root / block_size_image ) ) & """
" | sfdisk """" & target & """"""" )
  if 0 != cmd_result.exitCode:
    echo "Unable to set partition types in target image: " & cmd_result.output
    quit( 1 )

  # increment current offset
  current_offset += partition_table_offset
  # copy boot partition
  image_dd( boot_partition, target, -1, block_size, current_offset )
  # increment current offset
  current_offset += partition_size_boot
  # copy root partition
  image_dd( root_partition, target, -1, block_size, current_offset )
  # increment current offset
  current_offset += partition_size_root

proc create_plain_image_file*( image_type: string, root_path: string ): void =
  # bunch of variables
  let base_path: string = joinPath( getCurrentDir(), "tmp" )
  let image_path: string = joinPath( base_path, "image.img" )
  let boot_directory_path: string = joinPath( base_path, "partition", "boot" )
  let root_directory_path: string = joinPath( base_path, "partition", "root" )
  let root_etc_directory_path: string = joinPath( root_directory_path, "etc" )
  # create folder boot, root and etc in root image
  createDir( joinPath( root_directory_path, "boot" ) )
  createDir( root_etc_directory_path )
  createDir( joinPath( root_directory_path, "root" ) )
  # copy fstab to partition root etc folder
  copyFile( joinPath( getCurrentDir(), "file", image_type, "fstab" ), joinPath( root_etc_directory_path, "fstab" ) )
  if "raspi" == image_type:
    # copy necessary stuff to boot partition
    let config_file: string = joinPath( root_path, "build-aux", "platform", image_type, "config.txt" )
    let cmdline_file: string = joinPath( root_path, "build-aux", "platform", image_type, "cmdline.txt" )
    if fileExists( config_file ): copyFile( config_file, joinPath( boot_directory_path, "config.txt" ) )
    if fileExists( cmdline_file ): copyFile( cmdline_file, joinPath( boot_directory_path, "cmdline.txt" ) )
    # create boot partition
    image_create_partition( image_path, 256, boot_directory_path, "fat32", 100, root_directory_path, "ext2" )
    let destination_image_path: string = joinPath( root_path, "build-aux", "platform", image_type, "sdcard.img" )
    if fileExists( destination_image_path ): removeFile( destination_image_path )
    copyFile( image_path, destination_image_path )
