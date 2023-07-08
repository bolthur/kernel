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
import std/httpclient
import zippy/tarballs

proc onProgressChanged(total, progress, speed: BiggestInt): void =
  stdout.write "\rDownloaded ", progress, " of ", total, " - Current rate: ", speed div 1000, "kb/s"
  stdout.flushFile()

proc copyFileToBoot*( path: string, subfolder:string = "" ): void =
  if fileExists( path ):
    # create base path
    let basePath = joinPath( getCurrentDir(), "tmp", "partition", "boot" )
    createDir( basePath )
    if subfolder != "":
      createDir( joinPath( basePath, subfolder ) )
    # get folder path out of file
    let splitted = splitPath( path )
    if subfolder != "":
      copyFile( path, joinPath( basePath, subfolder, splitted.tail ) )
    else:
      copyFile( path, joinPath( basePath, splitted.tail ) )

proc loadFirmwareToBoot*( firmwareType: string ): void =
  let cachePath = joinPath( getCurrentDir(), ".cache" )
  createDir( cachePath )
  if "raspi" == firmwareType:
    # load firmware
    if not fileExists( joinPath( cachePath, "firmware.tar.gz" ) ):
      var client = newHttpClient()
      client.onProgressChanged = onProgressChanged
      client.downloadFile( "https://github.com/raspberrypi/firmware/archive/refs/tags/1.20230106.tar.gz", joinPath( cachePath, "firmware.tar.gz" ) )
      stdout.write "\r\n"
      stdout.flushFile()
      # unzip firmware
      extractAll( joinPath( cachePath, "firmware.tar.gz" ), joinPath( cachePath, "firmware" ) )
    # copy over to boot
    let basePath = joinPath( cachePath, "firmware", "firmware-1.20230106", "boot" )
    for file in walkDirRec( basePath, { pcFile, pcDir } ):
      let splitted = splitPath( file )
      if splitted.tail.startsWith( "kernel" ):
        continue
      var directory = splitted.head.replace(basePath, "")
      if directory != "":
        # replace possible starting directory separator
        if directory.startsWith(DirSep):
          directory.delete(0..0)
        # copy over to boot
        copyFileToBoot( file, directory )
      else:
        # copy over to boot
        copyFileToBoot( file )
