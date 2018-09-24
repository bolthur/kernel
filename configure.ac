
AC_PREREQ(2.57)
AC_INIT([myst-system-kernel], [0.1], [bug-kernel@myst-system.org])
AC_COPYRIGHT(copyright 2018 mist system)

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([src/kernel/main.c])
AM_INIT_AUTOMAKE([foreign 1.7])
AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([device],
  AC_HELP_STRING([--enable-device], [set target device (default and fallback is pc)]),
  [case "${enableval}" in
  rpi1_a)
    DEVICE="rpi1_a"
    ;;
  rpi1_a_plus)
    DEVICE="rpi1_a_plus"
    ;;
  rpi1_b)
    DEVICE="rpi1_b"
    ;;
  rpi1_b_plus)
    DEVICE="rpi1_b_plus"
    ;;
  rpi2_b)
    DEVICE="rpi2_b"
    ;;
  rpi2_b_rev2)
    DEVICE="rpi2_b_rev2"
    ;;
  rpi3_b)
    DEVICE="rpi3_b"
    ;;
  rpi3_b_plus)
    DEVICE="rpi3_b_plus"
    ;;
  rpi_zero)
    DEVICE="rpi_zero"
    ;;
  rpi_zero_w)
    DEVICE="rpi_zero_w"
    ;;
  esac],
  [DEVICE="rpi2_b"]
)

# set target and flags
MIST_SET_HOST

AC_ARG_ENABLE([debug],
  AC_HELP_STRING([--enable-debug], [generate debug symbols (default is NO)]),
  [CFLAGS="${CFLAGS} -g"]
)

AC_ARG_ENABLE([opt],
  AC_HELP_STRING([--disable-opt], [optimize code (default is YES)]),
  [case "${enableval}" in
  no | 0)
    CFLAGS="${CFLAGS} -O0"
    ;;
  1)
    CFLAGS="${CFLAGS} -O1"
    ;;
  2)
    CFLAGS="${CFLAGS} -O2"
    ;;
  3)
    CFLAGS="${CFLAGS} -O3"
    ;;
  *)
    CFLAGS="${CFLAGS} -O2"
    ;;
  esac],
  [CFLAGS="${CFLAGS} -O2"]
)

# set normal flags
MIST_SET_FLAGS

# extend path with local toolchain
srcdir2=`readlink -f $srcdir`
PATH="${srcdir2}/toolchain/opt/cross/bin:${PATH}"
AC_SUBST(PATH, "${srcdir2}/toolchain/opt/cross/bin:${PATH}")

# conftest
AC_LANG_CONFTEST([AC_LANG_SOURCE([[const char hw[] = "Hello, World\n";]])])

# set cc to clang
AC_PROG_CC([gcc])

# checks for programs
AC_PROG_CC
AM_PROG_AS
AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_CXX
MIST_PROG_OBJCOPY

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_RESTRICT

# Some compiler dependent features.
AC_DEFINE_UNQUOTED([PACKED], [__attribute__((packed))], [Keyword for packing structures.])
AC_DEFINE_UNQUOTED([SECTION(x)], [__attribute__((section(x)))], [Keyword for section placement.])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/kernel/Makefile
  src/kernel/arch/Makefile
  src/kernel/arch/arm/Makefile
  src/kernel/arch/arm/v6/Makefile
  src/kernel/arch/arm/v7/Makefile
  src/kernel/arch/arm/v8/Makefile
  src/kernel/vendor/Makefile
  src/kernel/vendor/rpi/Makefile
  src/libc/Makefile
])

AC_OUTPUT
