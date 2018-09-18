#!/bin/bash

# Package Verions
PKG_BINUTILS="2.30"
PKG_GCC="8.1.0"

# Get directory path
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"
CPU_COUNT=$(nproc)

# Create directory for compile output
mkdir -p "$BASEDIR/opt"
mkdir -p "$BASEDIR/src"

# Some generic exports for compilation
export PREFIX="$BASEDIR/opt/cross"
export PATH="$PREFIX/bin:$PATH"

# Switch to src dir
cd "$BASEDIR/src"

# Build arm cross compiler
export TARGET=arm-none-eabi

# Create build directories
mkdir -p "$BASEDIR/build/binutils-arm"
mkdir -p "$BASEDIR/build/gcc-arm"

# Download binutils and extract
if [ ! -f binutils-$PKG_BINUTILS.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/binutils/binutils-${PKG_BINUTILS}.tar.gz"
  tar -xzf binutils-${PKG_BINUTILS}.tar.gz

  # Build binutils
  cd "$BASEDIR/build/binutils-arm"
  ../../src/binutils-$PKG_BINUTILS/configure --target=$TARGET \
    --prefix="$PREFIX" \
    --with-sysroot \
    --disable-nls \
    --disable-werror
  make all -j${CPU_COUNT}
  make install
  cd "$BASEDIR/src"
fi

## Download gcc and extract
if [ ! -f gcc-$PKG_GCC.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/gcc/gcc-${PKG_GCC}/gcc-${PKG_GCC}.tar.gz"
  tar -xzf gcc-${PKG_GCC}.tar.gz
fi

## Build gcc
cd "$BASEDIR/build/gcc-arm"
../../src/gcc-$PKG_GCC/configure \
  --target=$TARGET \
  --prefix="$PREFIX" \
  --disable-nls \
  --enable-languages=c,c++ \
  --without-headers
make all-gcc -j${CPU_COUNT}
make all-target-libgcc -j${CPU_COUNT}
make install-gcc
make install-target-libgcc
cd "$BASEDIR/src"
