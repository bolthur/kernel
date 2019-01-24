#!/bin/bash

# Package Verions
PKG_BINUTILS="2.31.1"
PKG_GCC="8.2.0"
PKG_GMP="6.1.0"
PKG_MPFR="3.1.4"
PKG_MPC="1.0.3"
PKG_ISL="0.18"
PKG_GDB="8.2"

# Get directory path
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"
if [[ "$OSTYPE" == "darwin"* ]]; then
  CPU_COUNT=$(sysctl -n hw.physicalcpu)
else
  CPU_COUNT=$(nproc)
fi

# Create directory for compile output
mkdir -p "$BASEDIR/opt"
mkdir -p "$BASEDIR/src"

# Some generic exports for compilation
export PREFIX="$BASEDIR/opt/cross"
export PATH="$PREFIX/bin:$PATH"

# Switch to src dir
cd "$BASEDIR/src"

# Create build directories
mkdir -p "$BASEDIR/build/binutils-arm-none-eabi"
mkdir -p "$BASEDIR/build/gcc-arm-none-eabi"
mkdir -p "$BASEDIR/build/gdb-arm-none-eabi"
mkdir -p "$BASEDIR/build/binutils-aarch64-none-elf"
mkdir -p "$BASEDIR/build/gcc-aarch64-none-elf"
mkdir -p "$BASEDIR/build/gdb-aarch64-none-elf"

# mac related special
if [[ "$OSTYPE" == "darwin"* ]]; then
  mkdir -p "$BASEDIR/build/gmp"
  mkdir -p "$BASEDIR/build/mpfr"
  mkdir -p "$BASEDIR/build/mpc"
  mkdir -p "$BASEDIR/build/isl"
fi

# download mac related additional packages
if [[ "$OSTYPE" == "darwin"* ]]; then
  # gmp
  if [ ! -f gmp-$PKG_GMP.tar.bz2 ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/gmp-${PKG_GMP}.tar.bz2"
    tar -xjf gmp-$PKG_GMP.tar.bz2
  fi
  # mpfr
  if [ ! -f mpfr-$PKG_MPFR.tar.bz2 ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/mpfr-${PKG_MPFR}.tar.bz2"
    tar -xjf mpfr-$PKG_MPFR.tar.bz2
  fi
  # mpc
  if [ ! -f mpc-$PKG_MPC.tar.gz ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/mpc-${PKG_MPC}.tar.gz"
    tar -xzf mpc-$PKG_MPC.tar.gz
  fi
  # isl
  if [ ! -f isl-$PKG_ISL.tar.bz2 ]; then
    wget "https://gcc.gnu.org/pub/gcc/infrastructure/isl-${PKG_ISL}.tar.bz2"
    tar -xjf isl-${PKG_ISL}.tar.bz2
  fi
fi
# binutils
if [ ! -f binutils-$PKG_BINUTILS.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/binutils/binutils-${PKG_BINUTILS}.tar.gz"
  tar -xzf binutils-${PKG_BINUTILS}.tar.gz
fi
## gcc
if [ ! -f gcc-$PKG_GCC.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/gcc/gcc-${PKG_GCC}/gcc-${PKG_GCC}.tar.gz"
  tar -xzf gcc-${PKG_GCC}.tar.gz
fi
# gdb
if [ ! -f gdb-$PKG_GDB.tar.gz ]; then
  wget "https://ftp.gnu.org/gnu/gdb/gdb-${PKG_GDB}.tar.gz"
  tar -xzf gdb-${PKG_GDB}.tar.gz
fi

# mac related
if [[ "$OSTYPE" == "darwin"* ]]; then
  # configure gmp
  if [ ! -f "$BASEDIR/build/gmp/crosscompiler.configured" ]; then
    cd "$BASEDIR/build/gmp"
    ../../src/gmp-$PKG_GMP/configure \
      --prefix="$PREFIX"
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as configured
    touch "$BASEDIR/build/gmp/crosscompiler.configured"
    cd "$BASEDIR/src"
  fi
  # build gmp
  if [ ! -f "$BASEDIR/build/gmp/crosscompiler.built" ]; then
    cd "$BASEDIR/build/gmp"

    make all -j${CPU_COUNT}
    if [ $? -ne 0 ]; then
      exit 1
    fi

    make check
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as built
    touch "$BASEDIR/build/gmp/crosscompiler.built"
    cd "$BASEDIR/src"
  fi
  # install gmp
  if [ ! -f "$BASEDIR/build/gmp/crosscompiler.installed" ]; then
    cd "$BASEDIR/build/gmp"

    make install
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as installed
    touch "$BASEDIR/build/gmp/crosscompiler.installed"
    cd "$BASEDIR/src"
  fi



  # configure mpfr
  if [ ! -f "$BASEDIR/build/mpfr/crosscompiler.configured" ]; then
    cd "$BASEDIR/build/mpfr"
    ../../src/mpfr-$PKG_MPFR/configure \
      --prefix="$PREFIX" \
      --with-gmp=$PREFIX
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as configured
    touch "$BASEDIR/build/mpfr/crosscompiler.configured"
    cd "$BASEDIR/src"
  fi
  # build mpfr
  if [ ! -f "$BASEDIR/build/mpfr/crosscompiler.built" ]; then
    cd "$BASEDIR/build/mpfr"

    make all -j${CPU_COUNT}
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as built
    touch "$BASEDIR/build/mpfr/crosscompiler.built"
    cd "$BASEDIR/src"
  fi
  # install mpfr
  if [ ! -f "$BASEDIR/build/mpfr/crosscompiler.installed" ]; then
    cd "$BASEDIR/build/mpfr"

    make install
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as installed
    touch "$BASEDIR/build/mpfr/crosscompiler.installed"
    cd "$BASEDIR/src"
  fi



  # configure mpc
  if [ ! -f "$BASEDIR/build/mpc/crosscompiler.configured" ]; then
    cd "$BASEDIR/build/mpc"
    ../../src/mpc-$PKG_MPC/configure \
      --prefix="$PREFIX" \
      --with-gmp=$PREFIX \
      --with-mpf=$PREFIX
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as configured
    touch "$BASEDIR/build/mpc/crosscompiler.configured"
    cd "$BASEDIR/src"
  fi
  # build mpc
  if [ ! -f "$BASEDIR/build/mpc/crosscompiler.built" ]; then
    cd "$BASEDIR/build/mpc"

    make all -j${CPU_COUNT}
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as built
    touch "$BASEDIR/build/mpc/crosscompiler.built"
    cd "$BASEDIR/src"
  fi
  # install mpc
  if [ ! -f "$BASEDIR/build/mpc/crosscompiler.installed" ]; then
    cd "$BASEDIR/build/mpc"

    make install
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as installed
    touch "$BASEDIR/build/mpc/crosscompiler.installed"
    cd "$BASEDIR/src"
  fi



  # configure isl
  if [ ! -f "$BASEDIR/build/isl/crosscompiler.configured" ]; then
    cd "$BASEDIR/build/isl"
    ../../src/isl-$PKG_ISL/configure \
      --prefix="$PREFIX" \
      --with-gmp-prefix=$PREFIX
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as configured
    touch "$BASEDIR/build/isl/crosscompiler.configured"
    cd "$BASEDIR/src"
  fi
  # build isl
  if [ ! -f "$BASEDIR/build/isl/crosscompiler.built" ]; then
    cd "$BASEDIR/build/isl"

    make all -j${CPU_COUNT}
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as built
    touch "$BASEDIR/build/isl/crosscompiler.built"
    cd "$BASEDIR/src"
  fi
  # install isl
  if [ ! -f "$BASEDIR/build/isl/crosscompiler.installed" ]; then
    cd "$BASEDIR/build/isl"

    make install
    if [ $? -ne 0 ]; then
      exit 1
    fi

    # mark as installed
    touch "$BASEDIR/build/isl/crosscompiler.installed"
    cd "$BASEDIR/src"
  fi
fi



# configure binutils for target arm-none-eabi
if [ ! -f "$BASEDIR/build/binutils-arm-none-eabi/crosscompiler.configured" ]; then
  export TARGET=arm-none-eabi
  cd "$BASEDIR/build/binutils-arm-none-eabi"

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

  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as configured
  touch "$BASEDIR/build/binutils-arm-none-eabi/crosscompiler.configured"
  cd "$BASEDIR/src"
fi
# build binutils for target arm-none-eabi
if [ ! -f "$BASEDIR/build/binutils-arm-none-eabi/crosscompiler.built" ]; then
  cd "$BASEDIR/build/binutils-arm-none-eabi"

  make all -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as built
  touch "$BASEDIR/build/binutils-arm-none-eabi/crosscompiler.built"
  cd "$BASEDIR/src"
fi
# install binutils for target arm-none-eabi
if [ ! -f "$BASEDIR/build/binutils-arm-none-eabi/crosscompiler.installed" ]; then
  cd "$BASEDIR/build/binutils-arm-none-eabi"

  make install
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as installed
  touch "$BASEDIR/build/binutils-arm-none-eabi/crosscompiler.installed"
  cd "$BASEDIR/src"
fi



# configure binutils for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/binutils-aarch64-none-elf/crosscompiler.configured" ]; then
  export TARGET=aarch64-none-elf
  cd "$BASEDIR/build/binutils-aarch64-none-elf"

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

  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as configured
  touch "$BASEDIR/build/binutils-aarch64-none-elf/crosscompiler.configured"
  cd "$BASEDIR/src"
fi
# build binutils for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/binutils-aarch64-none-elf/crosscompiler.built" ]; then
  cd "$BASEDIR/build/binutils-aarch64-none-elf"

  make all -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as built
  touch "$BASEDIR/build/binutils-aarch64-none-elf/crosscompiler.built"
  cd "$BASEDIR/src"
fi
# install binutils for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/binutils-aarch64-none-elf/crosscompiler.installed" ]; then
  cd "$BASEDIR/build/binutils-aarch64-none-elf"

  make install
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as installed
  touch "$BASEDIR/build/binutils-aarch64-none-elf/crosscompiler.installed"
  cd "$BASEDIR/src"
fi



# configure gcc for target arm-none-eabi
if [ ! -f "$BASEDIR/build/gcc-arm-none-eabi/crosscompiler.configured" ]; then
  export TARGET=arm-none-eabi
  cd "$BASEDIR/build/gcc-arm-none-eabi"

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

  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as configured
  touch "$BASEDIR/build/gcc-arm-none-eabi/crosscompiler.configured"
  cd "$BASEDIR/src"
fi
# build gcc for target arm-none-eabi
if [ ! -f "$BASEDIR/build/gcc-arm-none-eabi/crosscompiler.built" ]; then
  cd "$BASEDIR/build/gcc-arm-none-eabi"

  make all-gcc -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  make all-target-libgcc -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as built
  touch "$BASEDIR/build/gcc-arm-none-eabi/crosscompiler.built"
  cd "$BASEDIR/src"
fi
# install gcc for target arm-none-eabi
if [ ! -f "$BASEDIR/build/gcc-arm-none-eabi/crosscompiler.installed" ]; then
  cd "$BASEDIR/build/gcc-arm-none-eabi"

  make install-gcc
  if [ $? -ne 0 ]; then
    exit 1
  fi

  make install-target-libgcc
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as installed
  touch "$BASEDIR/build/gcc-arm-none-eabi/crosscompiler.installed"
  cd "$BASEDIR/src"
fi



# configure gcc for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/gcc-aarch64-none-elf/crosscompiler.configured" ]; then
  export TARGET=aarch64-none-elf
  cd "$BASEDIR/build/gcc-aarch64-none-elf"

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

  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as configured
  touch "$BASEDIR/build/gcc-aarch64-none-elf/crosscompiler.configured"
  cd "$BASEDIR/src"
fi
# build gcc for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/gcc-aarch64-none-elf/crosscompiler.built" ]; then
  cd "$BASEDIR/build/gcc-aarch64-none-elf"

  make all-gcc -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  make all-target-libgcc -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as built
  touch "$BASEDIR/build/gcc-aarch64-none-elf/crosscompiler.built"
  cd "$BASEDIR/src"
fi
# install gcc for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/gcc-aarch64-none-elf/crosscompiler.installed" ]; then
  cd "$BASEDIR/build/gcc-aarch64-none-elf"

  make install-gcc
  if [ $? -ne 0 ]; then
    exit 1
  fi

  make install-target-libgcc
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as installed
  touch "$BASEDIR/build/gcc-aarch64-none-elf/crosscompiler.installed"
  cd "$BASEDIR/src"
fi



# configure gdb for target arm-none-eabi
if [ ! -f "$BASEDIR/build/gdb-arm-none-eabi/crosscompiler.configured" ]; then
  export TARGET=arm-none-eabi
  cd "$BASEDIR/build/gdb-arm-none-eabi"

  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
      --enable-interwork \
      --enable-multilib
  else
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX"
  fi

  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as configured
  touch "$BASEDIR/build/gdb-arm-none-eabi/crosscompiler.configured"
  cd "$BASEDIR/src"
fi
# build gdb for target arm-none-eabi
if [ ! -f "$BASEDIR/build/gdb-arm-none-eabi/crosscompiler.built" ]; then
  cd "$BASEDIR/build/gdb-arm-none-eabi"

  make all -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as built
  touch "$BASEDIR/build/gdb-arm-none-eabi/crosscompiler.built"
  cd "$BASEDIR/src"
fi
# install gdb for target arm-none-eabi
if [ ! -f "$BASEDIR/build/gdb-arm-none-eabi/crosscompiler.installed" ]; then
  cd "$BASEDIR/build/gdb-arm-none-eabi"

  make install
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as installed
  touch "$BASEDIR/build/gdb-arm-none-eabi/crosscompiler.installed"
  cd "$BASEDIR/src"
fi



# configure gdb for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/gdb-aarch64-none-elf/crosscompiler.configured" ]; then
  export TARGET=aarch64-none-elf
  cd "$BASEDIR/build/gdb-aarch64-none-elf"

  if [[ "$OSTYPE" == "darwin"* ]]; then
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX" \
      --with-gmp="$PREFIX" \
      --with-mpfr="$PREFIX" \
      --with-mpc="$PREFIX" \
      --enable-interwork \
      --enable-multilib
  else
    ../../src/gdb-$PKG_GDB/configure \
      --target=$TARGET \
      --prefix="$PREFIX"
  fi

  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as configured
  touch "$BASEDIR/build/gdb-aarch64-none-elf/crosscompiler.configured"
  cd "$BASEDIR/src"
fi
# build gdb for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/gdb-aarch64-none-elf/crosscompiler.built" ]; then
  cd "$BASEDIR/build/gdb-aarch64-none-elf"

  make all -j${CPU_COUNT}
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as built
  touch "$BASEDIR/build/gdb-aarch64-none-elf/crosscompiler.built"
  cd "$BASEDIR/src"
fi
# install gdb for target aarch64-none-elf
if [ ! -f "$BASEDIR/build/gdb-aarch64-none-elf/crosscompiler.installed" ]; then
  cd "$BASEDIR/build/gdb-aarch64-none-elf"

  make install
  if [ $? -ne 0 ]; then
    exit 1
  fi

  # mark as installed
  touch "$BASEDIR/build/gdb-aarch64-none-elf/crosscompiler.installed"
  cd "$BASEDIR/src"
fi
