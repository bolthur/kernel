#!/bin/bash

# get absolute script path
absolute_script_path=$(readlink -f "$0")
# get directory of script
script_directory=$(dirname "$absolute_script_path")
# get root project dir
project_directory=$(dirname "$script_directory")

# files to copy
kernel="$project_directory/build/bolthur/kernel/target/rpi/kernel7.img"
initrd="$project_directory/build-aux/platform/rpi/initrd"
# check for existance
if [ ! -e "$kernel" ]; then
  echo "kernel to copy not found under \"$kernel\""
  exit;
elif [ ! -e "$initrd" ]; then
  echo "initrd to copy not found under \"$initrd\""
  exit;
fi

# tmp folder path
tmp_directory="$script_directory/tmp"
# check for root privileges
if [ $(id -u) -ne 0 ]; then
  echo "script needs root privileges for mounting a device!"
  exec sudo -- "$0" "$@"
  exit "$?"
fi

# get device to mount from parameter
device="$1"
if [ -z "$device" ]; then
  echo "no device given!"
  exit 1
fi
# check if device file exists
if [ ! -e "$device" ]; then
  echo "device \"$device\" does not exist!"
  exit 1
fi

# existing tmp directory is an error
if [ -d "$tmp_directory" ]; then
  echo "tmp dir \"$tmp_directory\" already exists!"
  exit 1
fi

# create tmp dir
echo "Creating directory for mapping..."
mkdir -p $tmp_directory
if [ $? != 0 ]; then
  echo "unable to create directory!"
  exit 1
fi

# mount device
echo "Mounting device \"$device\" to \"$tmp_directory\"..."
mount $device $tmp_directory
if [ $? != 0 ]; then
  echo "error while mounting $device to $tmp_directory!"
  exit 1
fi

# copy kernel
echo "Copying kernel from \"$kernel\" to mount point at \"$tmp_directory\"..."
cp $kernel $tmp_directory/bolthur.img
if [ $? != 0 ]; then
  echo "error while copying kernel!"
  exit 1
fi

# copy initrd
echo "Copying initrd from \"$initrd\" to mount point at \"$tmp_directory\"..."
cp $initrd $tmp_directory
if [ $? != 0 ]; then
  echo "error while copying initrd!"
  exit 1
fi

# sync
echo "Performing sync..."
sync
if [ $? != 0 ]; then
  echo "sync failed!"
  exit 1
fi

# unmount again
echo "Unmounting \"$tmp_directory\"..."
umount $tmp_directory
if [ $? != 0 ]; then
  echo "error while mounting $device to $tmp_directory!"
  exit 1
fi

# sync
echo "Performing another sync..."
sync
if [ $? != 0 ]; then
  echo "sync failed!"
  exit 1
fi

# cleanup tmp dir
echo "Cleaning up tmp directory used for mapping..."
rm -rf $tmp_directory
if [ $? != 0 ]; then
  echo "unable to remove directory!"
  exit 1
fi

exit 0