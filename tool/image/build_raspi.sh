#!/bin/bash
set -x

# TODO: Allow specify of firmware via option
# TODO: Save firmware out of temporary
# 

# get absolute script path
absolute_script_path=$(readlink -f "$0")
# get directory of script
script_directory=$(dirname "$absolute_script_path")
# get root project dir
project_directory=$( dirname $(dirname "$script_directory"))

# check for root privileges
if [ $(id -u) -ne 0 ]; then
  echo "script needs root privileges for mounting a device!"
  exec sudo -- "$0" "$@"
  exit "$?"
fi

# files to copy
kernel_folder="$project_directory/build/bolthur/kernel/target/raspi"
kernel_possible_image=(kernel.img kernel7.img kernel8.img)
config="$project_directory/build-aux/platform/raspi/config.txt"
initrd="$project_directory/build-aux/platform/raspi/initrd"
# determine kernel to copy
kernel=""
for possible in "${kernel_possible_image[@]}"; do
  if [ -e "$kernel_folder/$possible" ]; then
    kernel="$kernel_folder/$possible"
    break
  fi
done
# check for existance
if [ ! -e "$kernel" ]; then
  echo "kernel to copy not found under \"$kernel\""
  exit;
elif [ ! -e "$initrd" ]; then
  echo "initrd to copy not found under \"$initrd\""
  exit;
elif [ ! -e "$config" ]; then
  echo "config.txt to copy not found under \"$config\""
  exit;
fi

# tmp folder path
tmp_directory="$script_directory/tmp"
tmp_directory_download="${tmp_directory}/download"
# treat existing tmp directory is an error
if [ -d "$tmp_directory" ]; then
  echo "tmp dir \"$tmp_directory\" already exists!"
  exit 1
fi
# create download necessary folders
mkdir -p $tmp_directory_download
if [ $? != 0 ]; then
  echo "unable to create download directory!"
  exit 1
fi
# load firmware release
wget -O ${tmp_directory_download}/firmware.tar.gz https://github.com/raspberrypi/firmware/archive/refs/tags/1.20220120.tar.gz
if [ $? != 0 ]; then
  echo "unable to download firmware!"
  exit 1
fi
# specify extract name
$firmware_extract_name="firmware"
# create extract folder
mkdir -p "$tmp_directory_download/$firmware_extract_name"
if [ $? != 0 ]; then
  echo "unable to create firmware extract folder!"
  exit 1
fi
# decompress firmware
tar -xzvf ${tmp_directory_download}/firmware.tar.gz -C "$tmp_directory_download/$firmware_extract_name" --strip-components=1
if [ $? != 0 ]; then
  echo "unable to extract firmware!"
  exit 1
fi

# defaults
image_size=128
# parse arguments
while getopts s: flag; do
  case "${flag}" in
    s) image_size=${OPTARG};;
  esac
done
# validate arguments
if ! [[ "$image_size" =~ ^[0-9]+$ ]]; then
  echo "Given image size is not numeric"
  exit 1
fi

# check for smaller size than minimum of 64
if [[ "$image_size" -lt 128 ]]; then
  echo "Min image size of 128 is required"
  exit 1
fi
# Validate image size to check whether it's a known size
check_float=$(echo "$image_size/8/8" | bc -l)
check_integer=$((image_size/8/8))
check_remaining=$(echo "$check_float - $check_integer" | bc -l)
check_result=$(echo "$check_remaining > 0" | bc -l)
# in case the check result is not 0, than the size is not in a valid format, 
# e.g. 32, 64, 128, ...
if [[ "$check_result" -ne 0 ]]; then
  echo "Invalid image size provided"
  exit 1
fi

# image destination directory
image_file="${project_directory}/build-aux/platform/raspi/sdcard.img"
# boot partition is fixed to 60M
image_partition_boot_size="60M"
# root partition shall take the rest
image_partition_root_size="$((image_size-60))M"

# create sd card image below tmp folder
tmp_image_file="${tmp_directory}/sdcard.img"
tmp_image_boot="${tmp_directory}/mnt/boot"
tmp_image_root="${tmp_directory}/mnt/root"
# create necessary folders
mkdir -p $tmp_directory
if [ $? != 0 ]; then
  echo "unable to create temporary directory!"
  exit 1
fi
mkdir -p $tmp_image_boot
if [ $? != 0 ]; then
  echo "unable to create temporary boot directory!"
  exit 1
fi
mkdir -p $tmp_image_root
if [ $? != 0 ]; then
  echo "unable to create temporary root directory!"
  exit 1
fi

# create plain image file
dd if=/dev/zero of="${tmp_image_file}" bs=1M count="${image_size}"
if [ $? != 0 ]; then
  echo "unable to create plain image file in tmp directory!"
  exit 1
fi

# setup loopback device
ld=$(losetup --show -fP $tmp_image_file)
if [ -z "$ld" ]; then
  echo "no loopback device returned from losetup"
  exit 1
fi

# create partitions
# to create the partitions programatically (rather than manually)
# we're going to simulate the manual input to fdisk
# The sed script strips off all the comments so that we can 
# document what we're doing in-line with the actual commands
# Note that a blank line (commented as "defualt" will send a empty
# line terminated with a newline to take the fdisk default.
# o # clear the in memory partition table
sed -e 's/\s*\([\+0-9a-zA-Z]*\).*/\1/' << EOF | fdisk ${ld}
  n # new partition
  p # primary partition
  1 # partition number 1
    # default - start at beginning of disk 
  +${image_partition_boot_size} # boot parttion
  n # new partition
  p # primary partition
  2 # partion number 2
    # default, start immediately after preceding partition
    # default, extend partition to end of disk
  a # make a partition bootable
  1 # bootable partition is partition 1
  t # change partition type
  1 # change first partition
  0c # change to FAT32 LBA
  p # print the in-memory partition table
  w # write the partition table
  q # and we're done
EOF
if [ $? != 0 ]; then
  echo "unable to create partitions with loop back device"
  losetup -d $ld
  exit 1
fi
#format loopback partitions
mkfs.vfat -F32 "${ld}p1"
if [ $? != 0 ]; then
  echo "unable to partition boot with fat"
  losetup -d $ld
  exit 1
fi
#format loopback partitions
mkfs -t ext2 "${ld}p2"
if [ $? != 0 ]; then
  echo "unable to partition root with ext2"
  losetup -d $ld
  exit 1
fi
# mount boot partition
mount "${ld}p1" $tmp_image_boot
if [ $? != 0 ]; then
  echo "error while mounting ${ld}p1 to $tmp_image_boot!"
  losetup -d $ld
  exit 1
fi
# copy boot stuff from firmware
cp -r $tmp_directory_download/$firmware_extract_name/boot/* $tmp_image_boot
if [ $? != 0 ]; then
  echo "error while copying boot stuff from firmware to mount point!"
  umount $tmp_image_boot
  losetup -d $ld
  exit 1
fi
# remove all .img files from boot
rm $tmp_image_boot/*.img
if [ $? != 0 ]; then
  echo "error while cleaning up boot mount point!"
  umount $tmp_image_boot
  losetup -d $ld
  exit 1
fi
# copy necessary files to boot partition
file_copy_list=("$kernel" "$config" "$initrd")
# copy over config.txt initrd and kernel7
for to_copy in "${file_copy_list[@]}"; do
  cp $to_copy $tmp_image_boot/
done
# unmount boot partition
umount $tmp_image_boot
if [ $? != 0 ]; then
  echo "error while unmounting $tmp_image_boot!"
  umount $tmp_image_boot
  losetup -d $ld
  exit 1
fi

# FIXME: mount root partition
# FIXME: populate sysroot
# FIXME: umount root partition

# remove loopback device again
losetup -d $ld
if [ $? != 0 ]; then
  echo "error while removing loopback device $ld"
  exit 1
fi

# copying image to build-aux
cp $tmp_image_file $project_directory/build-aux/platform/raspi/sdcard.img
if [ $? != 0 ]; then
  echo "Failed to copy built image file to build-aux"
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
