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
import std/strutils
import std/httpclient
import zippy/tarballs

proc onProgressChanged(total, progress, speed: BiggestInt): void =
  stdout.write "\rDownloaded ", progress, " of ", total, " - Current rate: ", speed div 1000, "kb/s"
  stdout.flushFile()

proc copy_file_to_boot*( path: string ): void =
  if fileExists( path ):
    # create base path
    let base_path = joinPath( getCurrentDir(), "tmp", "partition", "boot" )
    createDir( base_path )
    # get folder path out of file
    let splitted = splitPath( path )
    copyFile( path, joinPath( base_path, splitted.tail ) )

proc load_firmware_to_boot*( firmware_type: string ): void =
  let cache_path = joinPath( getCurrentDir(), ".cache" )
  createDir( cache_path )
  if "raspi" == firmware_type:
    # load firmware
    if not fileExists( joinPath( cache_path, "firmware.tar.gz" ) ):
      echo "Loading firmware for raspi"
      var client = newHttpClient()
      client.onProgressChanged = onProgressChanged
      client.downloadFile( "https://github.com/raspberrypi/firmware/archive/refs/tags/1.20220120.tar.gz", joinPath( cache_path, "firmware.tar.gz" ) )
      stdout.write "\r\n"
      stdout.flushFile()
      # unzip firmware
      echo "Unzipping firmware"
      extractAll( joinPath( cache_path, "firmware.tar.gz" ), joinPath( cache_path, "firmware" ) )
    echo "Copying firmware to boot folder"
    # copy over to boot
    for file in walkDirRec( joinPath( cache_path, "firmware", "firmware-1.20220120", "boot" ), { pcFile } ):
      let splitted = splitPath( file )
      if splitted.tail.startsWith( "kernel" ):
        continue
      # copy over to boot
      copy_file_to_boot( file )
