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
import std/strformat
import std/parsecfg
import std/sequtils
import zippy

proc scanDirectory*( path: string, fileType: string, additionalInfo: string, sysroot: string, compress: bool ): void =
  # loop through files of folder including subfolders
  for file in walkDirRec( path, { pcFile, pcLinkToFile } ):
    var fileToCheck = file
    var info = getFileInfo( file, false )
    var symlinkSrc = ""
    if pcLinkToFile == info.kind:
      # get info with follow symlink again
      info = getFileInfo( file )
      # skip non files
      if pcFile != info.kind: continue
      var splitted = file.splitFile()
      # expand symlink and use for check
      fileToCheck = expandSymlink( file )
      if not fileToCheck.isAbsolute:
        fileToCheck = joinPath( splitted.dir, fileToCheck )
      # determine symlink source and strip variable
      symlinkSrc = strip( replace( fileToCheck, splitted.dir & DirSep, "" ) )
      # loop as long as file to check is still a symlink
      info = getFileInfo( fileToCheck, false )
      while pcLinkToFile == info.kind:
        # expand symlink again
        var splitted = fileToCheck.splitFile()
        fileToCheck = expandSymlink( fileToCheck )
        # add absolute path
        if not fileToCheck.isAbsolute:
          fileToCheck = joinPath( splitted.dir, fileToCheck )
        # get info again
        info = getFileInfo( fileToCheck, false )
    let cmdResult: tuple[output: string, exitCode: int] = execCmdEx( "file " & fileToCheck )
    if 0 != cmdResult.exitCode:
      echo "unable to query info: " & cmdResult.output
      quit( 1 )
    if contains( cmdResult.output, fileType ) and contains( cmdResult.output, additionalInfo ):
      # split file path
      let split = splitPath( file )
      let splittedHead = split.head.split( DirSep )
      let executable = split.tail

      # get destination information
      var infoFile = fmt"{$DirSep}";
      for idx, value in splittedHead:
        if "build" == value: continue
        infoFile = joinPath( infoFile, value )
      infoFile = joinPath( infoFile, "config.ini" )
      if fileExists( infoFile ):
        # load config
        var dict = loadConfig( infoFile )
        # get necessary information
        let placeType = dict.getSectionValue("general", "type")
        var innerPath = dict.getSectionValue("general", "path")
        var innerPathSplitted = innerPath.split( "/" )
        # skip ramdisk without inner path
        if "ramdisk" == placeType and 0 >= len( innerPath ): continue

        var basePath = ""
        var compressFlag = compress
        if "ramdisk_compressed" == placeType:
          basePath = joinPath( getCurrentDir(), "tmp", "ramdisk", "compressed", joinPath( innerPathSplitted ) )
        elif "ramdisk_uncompressed" == placeType:
          basePath = joinPath( getCurrentDir(), "tmp", "ramdisk" )
        elif "image_root" == placeType:
          basePath = joinPath( getCurrentDir(), "tmp", "partition", "root", joinPath( innerPathSplitted ) )
          compressFlag = false
        elif "image_boot" == placeType:
          basePath = joinPath( getCurrentDir(), "tmp", "partition", "boot", joinPath( innerPathSplitted ) )
          compressFlag = false
        else:
          echo "unknown type ", $placeType , " for ", file, " with path ", innerPath
          continue

        createDir( basePath )
        if not isEmptyOrWhitespace( symlinkSrc ):
          createSymlink( symlinkSrc, joinPath( basePath, executable ) )
        else:
          if "ramdisk_uncompressed" == placeType:
            copyFile( file, joinPath( basePath, executable ) )
          else:
            if compressFlag:
              writeFile( joinPath( basePath, executable & ".gz" ), zippy.compress( readFile( file ), zippy.DefaultCompression, zippy.dfGzip ) )
            else:
              copyFile( file, joinPath( basePath, executable ) )
        continue

      if -1 != path.find(sysroot):
        # determine target folder
        var targetFolder = $DirSep
        var last = len( splittedHead ) - 1
        if executable == splittedHead[ last ]: last = last - 1

        var pos = -1
        for idx, value in splittedHead:
          if value == "application" or value == "server" or value == "usr" or value == "font" or value == "bosl":
            pos = idx
        if pos != -1:
          if pos + 1 < len( splittedHead ) and "platform" == splittedHead[ pos + 1 ]:
            # strip out platform and platform name from path
            var tmpInfo = splittedHead
            let first = pos + 1;
            let last = pos + 2;
            tmpInfo.delete( first..last )
            if tmpInfo.contains( "bosl" ):
              targetFolder &= tmpInfo[ pos..^1 ].join( $DirSep )
            else:
              targetFolder &= tmpInfo[ pos..^2 ].join( $DirSep )
          else:
            targetFolder &= splittedHead[ pos..^1 ].join( $DirSep )
        else:
          pos = -1

        var basePath = joinPath( getCurrentDir(), "tmp", "partition", "root", targetFolder )

        # symlink handling
        if not isEmptyOrWhitespace( symlinkSrc ):
          createDir( basePath )
          createSymlink( symlinkSrc, joinPath( basePath, executable ) )
        else:
          copyFile( file, joinPath( basePath, executable ) )
