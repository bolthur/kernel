#!/bin/bash
set -ex

# Get base directory
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"

# Switch to base dir
cd $BASEDIR

# Delete build directory if existing
if [ -d "build-pi2_rev1" ]; then
  rm -rf ./build-pi
fi

# Create build directory and switch into it
mkdir build-pi2_rev1
cd build-pi2_rev1

# Configure for build with all possible options
../../../../configure \
  --host arm-none-eabi \
  --enable-device=rpi2_b_rev1 \
  --enable-output \
  --enable-framebuffer-tty \
  --enable-serial-tty \
  --enable-output-mm-phys \
  --enable-output-mm-virt \
  --enable-output-mm-heap \
  --enable-output-mm-placement \
  --enable-output-mailbox \
  --enable-output-timer \

# Start build
make clean
make all
