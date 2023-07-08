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
import std/strutils
import std/math
import std/osproc

var mega: int = int( pow( 2.0, 20.0 ) )

proc imageDd( source: string, target: string, size: int, blockSize: int, offset: int ): void =
  var cmd:string = "dd if=" & source & " of=" & target & " bs=" & intToStr( blockSize ) & " count=" & intToStr( int( size / blockSize ) ) & " skip=" & intToStr( int( offset / blockSize ) )
  if -1 == size:
    cmd = "dd if=" & source & " of=" & target & " bs=" & intToStr( blockSize ) & " seek=" & intToStr( int( offset / blockSize ) )

  let result: tuple[output: string, exitCode: int] = execCmdEx( cmd )
  if 0 != result.exitCode:
    echo "Unable to create raw image: " & result.output
    quit( 1 )

proc imageCreatePartition( target: string, totalSize: int, bootFolder: string, bootType: string, bootSize: int, rootFolder: string, rootType: string ): void =
  # save partition table offset
  let partitionTableOffset: int = mega
  # use boot size - 1 for mbr
  let partitionSizeBoot: int = bootSize * mega
  # calculate partition root size
  let partitionSizeRoot: int = ( totalSize - bootSize ) * mega - partitionTableOffset

  var cmdResult: tuple[output: string, exitCode: int];

  let blockSizeImage: int = 512
  let blockSize: int = 1024
  var currentOffset: int = 0

  let basePath: string = joinPath( getCurrentDir(), "tmp" )
  let bootPartition: string = joinPath( basePath, "boot.img" )
  let rootPartition: string = joinPath( basePath, "root.img" )

  # create raw files for boot, root and destination image
  imageDd( "/dev/zero", bootPartition, partitionSizeBoot, blockSize, currentOffset )
  imageDd( "/dev/zero", rootPartition, partitionSizeRoot, blockSize, currentOffset )
  imageDd( "/dev/zero", target, partitionTableOffset + partitionSizeBoot + partitionSizeRoot, blockSize, currentOffset )
  # format boot image with fat 32
  cmdResult = execCmdEx( "mkfs.vfat -F32 " & bootPartition & " --mbr=no" )
  if 0 != cmdResult.exitCode:
    echo "Unable to format boot partition with FAT32: " & cmdResult.output
    quit( 1 )
  # format root image with ext2
  cmdResult = execCmdEx( """mke2fs -t ext2 -d """" & joinPath( basePath, "partition", "root" ) & """" -r 1 -N 0 -m 5 """" & rootPartition & """" """" & intToStr( int( partitionSizeRoot / mega ) ) & """M"""" )
  if 0 != cmdResult.exitCode:
    echo "Unable to format root partition with ext2: " & cmdResult.output
    quit( 1 )
  # copy files to boot partition
  for file in walkDirRec( joinPath( basePath, "partition", "boot" ), { pcFile, pcDir } ):
    var info = getFileInfo( file )
    if info.kind == pcDir:
      var localfolder = file.replace( joinPath( basePath, "partition", "boot" ) )
      cmdResult = execCmdEx( "mmd -i " & bootPartition & " ::" & localfolder )
    else:
      cmdResult = execCmdEx( "mcopy -i " & bootPartition & " " & file & " ::" )
    if 0 != cmdResult.exitCode:
      echo "Unable to copy content to boot partition: " & cmdResult.output
      quit( 1 )
  # FIXME: FIX USER IN GENERATED ext2 IMAGE
  # set partitions
  cmdResult = execCmdEx( """printf "
type=0c, size=""" & intToStr( int( partitionSizeBoot / blockSizeImage ) ) & """

type=83, size=""" & intToStr( int( partitionSizeRoot / blockSizeImage ) ) & """
" | sfdisk """" & target & """"""" )
  if 0 != cmdResult.exitCode:
    echo "Unable to set partition types in target image: " & cmdResult.output
    quit( 1 )

  # increment current offset
  currentOffset += partitionTableOffset
  # copy boot partition
  imageDd( bootPartition, target, -1, blockSize, currentOffset )
  # increment current offset
  currentOffset += partitionSizeBoot
  # copy root partition
  imageDd( rootPartition, target, -1, blockSize, currentOffset )
  # increment current offset
  currentOffset += partitionSizeRoot

proc createPlainImageFile*( imageType: string, rootPath: string ): void =
  # bunch of variables
  let basePath: string = joinPath( getCurrentDir(), "tmp" )
  let imagePath: string = joinPath( basePath, "image.img" )
  let bootDirectoryPath: string = joinPath( basePath, "partition", "boot" )
  let rootDirectoryPath: string = joinPath( basePath, "partition", "root" )
  let rootEtcDirectoryPath: string = joinPath( rootDirectoryPath, "etc" )
  # create folder boot, root and etc in root image
  createDir( joinPath( rootDirectoryPath, "boot" ) )
  createDir( joinPath( rootDirectoryPath, "ramdisk" ) )
  createDir( rootEtcDirectoryPath )
  createDir( joinPath( rootDirectoryPath, "root" ) )
  # copy default root folder stuff
  let rootFileStuff: string = joinPath( getCurrentDir(), "file", imageType, "root" )
  for file in walkDirRec( rootFileStuff, { pcFile } ):
    var fileDestination = file.replace( rootFileStuff, "" )
    copyFile( file, joinPath(rootDirectoryPath, fileDestination ) )
  if "raspi" == imageType:
    # copy necessary stuff to boot partition
    let configFile: string = joinPath( rootPath, "build-aux", "platform", imageType, "config.txt" )
    let cmdlineFile: string = joinPath( rootPath, "build-aux", "platform", imageType, "cmdline.txt" )
    if fileExists( configFile ): copyFile( configFile, joinPath( bootDirectoryPath, "config.txt" ) )
    if fileExists( cmdlineFile ): copyFile( cmdlineFile, joinPath( bootDirectoryPath, "cmdline.txt" ) )
    # create boot partition
    imageCreatePartition( imagePath, 1024, bootDirectoryPath, "fat32", 256, rootDirectoryPath, "ext2" )
    let destinationImagePath: string = joinPath( rootPath, "build-aux", "platform", imageType, "sdcard.img" )
    if fileExists( destinationImagePath ): removeFile( destinationImagePath )
    copyFile( imagePath, destinationImagePath )
