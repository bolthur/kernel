#!/bin/bash

# Package Verions
PKG_BINUTILS="2.30"
PKG_GCC="8.1.0"
PKG_GMP="6.1.0"
PKG_MPFR="3.1.4"
PKG_MPC="1.0.3"
PKG_ISL="0.18"
PKG_GDB="8.2"

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

# Create build directories
mkdir -p "$BASEDIR/build/binutils-arm-aarch32"
mkdir -p "$BASEDIR/build/gcc-arm-aarch32"
mkdir -p "$BASEDIR/build/gdb-arm-aarch32"
mkdir -p "$BASEDIR/build/binutils-arm-aarch64"
mkdir -p "$BASEDIR/build/gcc-arm-aarch64"
mkdir -p "$BASEDIR/build/gdb-arm-aarch64"

# mac related special
if [[ "$OSTYPE" == "darwin"* ]]; then
  mkdir -p "$BASEDIR/build/gmp"
  mkdir -p "$BASEDIR/build/mpfr"
  mkdir -p "$BASEDIR/build/mpc"
  mkdir -p "$BASEDIR/build/isl"
fi

# mac related
if [[ "$OSTYPE" == "darwin"* ]]; then
  # gmp related
  if [ ! -f gmp-$PKG_GMP.tar.bz2 ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/gmp-${PKG_GMP}.tar.bz2"
    tar -xjf gmp-$PKG_GMP.tar.bz2

    # Build gmp
    cd "$BASEDIR/build/gmp"
    ../../src/gmp-$PKG_GMP/configure \
      --prefix="$PREFIX"
    make all -j${CPU_COUNT}
    make check
    make install
    cd "$BASEDIR/src"
  fi

  # mpfr related
  if [ ! -f mpfr-$PKG_MPFR.tar.bz2 ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/mpfr-${PKG_MPFR}.tar.bz2"
    tar -xjf mpfr-$PKG_MPFR.tar.bz2

    # Build mpfr
    cd "$BASEDIR/build/mpfr"
    ../../src/mpfr-$PKG_MPFR/configure \
      --prefix="$PREFIX" \
      --with-gmp=$PREFIX
    make all -j${CPU_COUNT}
    make install
    cd "$BASEDIR/src"
  fi

  # mpc related
  if [ ! -f mpc-$PKG_MPC.tar.gz ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/mpc-${PKG_MPC}.tar.gz"
    tar -xzf mpc-$PKG_MPC.tar.gz

    # Build mpc
    cd "$BASEDIR/build/mpc"
    ../../src/mpc-$PKG_MPC/configure \
      --prefix="$PREFIX" \
      --with-gmp=$PREFIX \
      --with-mpf=$PREFIX
    make all -j${CPU_COUNT}
    make install
    cd "$BASEDIR/src"
  fi

  # Download isl and extract
  if [ ! -f isl-$PKG_ISL.tar.bz2 ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/isl-${PKG_ISL}.tar.bz2"
    tar -xjf isl-${PKG_ISL}.tar.bz2

    # Build isl
    cd "$BASEDIR/build/isl"
    ../../src/isl-$PKG_ISL/configure \
      --prefix="$PREFIX" \
      --with-gmp-prefix=$PREFIX
    make all -j${CPU_COUNT}
    make install
    cd "$BASEDIR/src"
  fi
fi

# Download binutils and extract
if [ ! -f binutils-$PKG_BINUTILS.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/binutils/binutils-${PKG_BINUTILS}.tar.gz"
  tar -xzf binutils-${PKG_BINUTILS}.tar.gz

  # Build binutils for aarch32
  export TARGET=arm-none-eabi
  cd "$BASEDIR/build/binutils-arm-aarch32"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/binutils-$PKG_BINUTILS/configure --target=$TARGET \
      --prefix="$PREFIX" \
      --with-sysroot \
      --disable-nls \
      --disable-werror \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
      --with-isl="$PREFIX" \
      --enable-interwork \
      --enable-multilib
  else
    ../../src/binutils-$PKG_BINUTILS/configure --target=$TARGET \
      --prefix="$PREFIX" \
      --with-sysroot \
      --disable-nls \
      --disable-werror
  fi
  make clean
  make all -j${CPU_COUNT}
  make install

  # Build binutils for aarch64
  export TARGET=aarch64-none-elf
  cd "$BASEDIR/build/binutils-arm-aarch64"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/binutils-$PKG_BINUTILS/configure --target=$TARGET \
      --prefix="$PREFIX" \
      --with-sysroot \
      --disable-nls \
      --disable-werror \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
      --with-isl="$PREFIX" \
      --enable-interwork \
      --enable-multilib
  else
    ../../src/binutils-$PKG_BINUTILS/configure --target=$TARGET \
      --prefix="$PREFIX" \
      --with-sysroot \
      --disable-nls \
      --disable-werror
  fi
  make clean
  make all -j${CPU_COUNT}
  make install
  cd "$BASEDIR/src"
fi

## Download gcc and extract
if [ ! -f gcc-$PKG_GCC.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/gcc/gcc-${PKG_GCC}/gcc-${PKG_GCC}.tar.gz"
  tar -xzf gcc-${PKG_GCC}.tar.gz

  # Build gcc for aarch32
  export TARGET=arm-none-eabi
  cd "$BASEDIR/build/gcc-arm-aarch32"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/gcc-$PKG_GCC/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --disable-nls \
      --enable-languages=c,c++ \
      --without-headers \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
      --enable-interwork \
      --enable-multilib
  else
    ../../src/gcc-$PKG_GCC/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --disable-nls \
      --enable-languages=c,c++ \
      --without-headers
  fi
  make all-gcc -j${CPU_COUNT}
  make all-target-libgcc -j${CPU_COUNT}
  make install-gcc
  make install-target-libgcc

  # Build gcc for aarch64
  export TARGET=aarch64-none-elf
  cd "$BASEDIR/build/gcc-arm-aarch64"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/gcc-$PKG_GCC/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --disable-nls \
      --enable-languages=c,c++ \
      --without-headers \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
      --enable-interwork \
      --enable-multilib
  else
    ../../src/gcc-$PKG_GCC/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --disable-nls \
      --enable-languages=c,c++ \
      --without-headers
  fi
  make all-gcc -j${CPU_COUNT}
  make all-target-libgcc -j${CPU_COUNT}
  make install-gcc
  make install-target-libgcc
  cd "$BASEDIR/src"
fi


## Download gcc and extract
if [ ! -f gdb-$PKG_GDB.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/gdb/gdb-${PKG_GDB}.tar.gz"
  tar -xzf gdb-${PKG_GDB}.tar.gz

  # Build gdb for aarch32
  export TARGET=arm-none-eabi
  cd "$BASEDIR/build/gdb-arm-aarch32"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
  else
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX"
  fi
  make all -j${CPU_COUNT}
  make install

  # Build gdb for aarch64
  export TARGET=aarch64-none-elf
  cd "$BASEDIR/build/gdb-arm-aarch64"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
  else
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX"
  fi
  make all -j${CPU_COUNT}
  make install
  cd "$BASEDIR/src"
fi
