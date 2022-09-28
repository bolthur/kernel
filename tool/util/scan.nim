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
import std/strformat
import std/parsecfg
import std/sequtils
import zippy

proc scan_directory*( path: string, file_type: string, additional_info: string, sysroot: string, compress: bool ): void =
  # loop through files of folder including subfolders
  for file in walkDirRec( path, { pcFile, pcLinkToFile } ):
    var file_to_check = file
    var info = getFileInfo( file, false )
    var symlink_src = ""
    if pcLinkToFile == info.kind:
      # get info with follow symlink again
      info = getFileInfo( file )
      # skip non files
      if pcFile != info.kind: continue
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
    if contains( outp_shell, file_type ) and contains( outp_shell, additional_info ):
      # split file path
      let split = splitPath( file )
      let splitted_head = split.head.split( DirSep )
      let executable = split.tail

      # get destination information
      var info_file = fmt"{$DirSep}";
      for idx, value in splitted_head:
        if "build" == value: continue
        info_file = join_path( info_file, value )
      info_file = join_path( info_file, "config.ini" )
      if fileExists( info_file ):
        # load config
        var dict = loadConfig( info_file )
        # get necessary information
        let place_type = dict.getSectionValue("general", "type")
        var inner_path = dict.getSectionValue("general", "path")
        var inner_path_splitted = inner_path.split( "/" )
        # skip ramdisk without inner path
        if "ramdisk" == place_type and 0 >= len( inner_path ): continue

        var base_path = ""
        var compress_flag = compress
        if "ramdisk_compressed" == place_type:
          base_path = joinPath( getCurrentDir(), "tmp", "ramdisk", "compressed", joinPath( inner_path_splitted ) )
        elif "ramdisk_uncompressed" == place_type:
          base_path = joinPath( getCurrentDir(), "tmp", "ramdisk" )
        elif "image_root" == place_type:
          base_path = joinPath( getCurrentDir(), "tmp", "partition", "root", joinPath( inner_path_splitted ) )
          compress_flag = false
        elif "image_boot" == place_type:
          base_path = joinPath( getCurrentDir(), "tmp", "partition", "boot", joinPath( inner_path_splitted ) )
          compress_flag = false
        else:
          echo "unknown type ", $place_type , " for ", file, " with path ", inner_path
          continue

        createDir( base_path )
        if not isEmptyOrWhitespace( symlink_src ):
          echo symlink_src
          createSymlink( symlink_src, joinPath( base_path, executable ) )
        else:
          if "ramdisk_uncompressed" == place_type:
            copyFile( file, joinPath( base_path, executable ) )
          else:
            if compress_flag:
              writeFile( joinPath( base_path, executable & ".gz" ), zippy.compress( readFile( file ), zippy.DefaultCompression, zippy.dfGzip ) )
            else:
              copyFile( file, joinPath( base_path, executable ) )
        continue

      if -1 != path.find(sysroot):
        # determine target folder
        var target_folder = $DirSep
        var last = len( splitted_head ) - 1
        if executable == splitted_head[ last ]: last = last - 1

        var pos = -1
        for idx, value in splitted_head:
          if value == "application" or value == "server" or value == "usr" or value == "font" or value == "bosl":
            pos = idx
        if pos != -1:
          if pos + 1 < len( splitted_head ) and "platform" == splitted_head[ pos + 1 ]:
            # strip out platform and platform name from path
            var tmp_info = splitted_head
            let first = pos + 1;
            let last = pos + 2;
            tmp_info.delete( first..last )
            if tmp_info.contains( "bosl" ):
              target_folder &= tmp_info[ pos..^1 ].join( $DirSep )
            else:
              target_folder &= tmp_info[ pos..^2 ].join( $DirSep )
          else:
            target_folder &= splitted_head[ pos..^1 ].join( $DirSep )
        else:
          pos = -1

        var base_path = joinPath( getCurrentDir(), "tmp", "partition", "root", target_folder )

        # symlink handling
        if not isEmptyOrWhitespace( symlink_src ):
          createDir( base_path )
          createSymlink( symlink_src, joinPath( base_path, executable ) )
        else:
          copyFile( file, joinPath( base_path, executable ) )
