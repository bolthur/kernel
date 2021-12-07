#!/bin/bash
set -x

# get absolute script path
absolute_script_path=$(readlink -f "$0")
# get directory of script
script_directory=$(dirname "$absolute_script_path")

# tmp folder path
tmp_directory="$script_directory/tmp"
# use sudo if there are no root privileges
cmd_suffix=""
if [ "$EUID" -ne 0 ]; then
  cmd_suffix="sudo"
fi

# get device to mount from parameter
device="$1"
if [ -z "$device" ]; then
  echo "no device given!"
  exit
fi
# check if device file exists
if [ ! -e "$device" ]; then
  echo "device \"$device\" does not exist!"
  exit
fi

# existing tmp directory is an error
if [ -d "$tmp_directory" ]; then
  echo "tmp dir \"$tmp_directory\" already exists!"
  exit
fi
# create tmp dir
mkdir -p $tmp_directory
# mount device
$cmd_suffix mount $device $tmp_directory
# copy kernel


echo "$script_directory"
echo "$tmp_directory"
echo "$cmd_suffix"
