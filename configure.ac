
AC_PREREQ(2.57)
AC_INIT([bolthur-kernel], [0.1], [https://github.com/bolthur/kernel/issues], [bolthur-kernel], [https://github.com/bolthur/kernel])
AC_COPYRIGHT([Copyright (C) 2018 - 2019 bolthur project])

AC_CONFIG_AUX_DIR([build-aux/config])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([src/kernel/main.c])
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS([include/config.h])

#AM_SILENT_RULES([yes])

AC_LANG([C])

# FIXME: Shouldn't be necessary, when creating symlinks of toolchain binaries to /usr/local/bin
AC_SUBST(PATH, "${HOME}/.bolthur/cross/bin:${PATH}")

AC_ARG_ENABLE([device],
  AC_HELP_STRING([--enable-device], [set target device (default and fallback is rpi2_b_rev1)]),
  [case "${enableval}" in
  rpi2_b_rev1)
    DEVICE="rpi2_b_rev1"
    ;;
  rpi3_b)
    DEVICE="rpi3_b"
    ;;
  rpi_zero_w)
    DEVICE="rpi_zero_w"
    ;;
  esac],
  [DEVICE="rpi2_b_rev1"]
)

AC_ARG_ENABLE([debug],
  AC_HELP_STRING([--enable-debug], [enable debug mode with debug symbols (default: off)])
)

AC_ARG_ENABLE([output],
  AC_HELP_STRING([--enable-output], [enable kernel output (default: off)])
)

AC_ARG_ENABLE([output-mm-phys],
  AC_HELP_STRING([--enable-output-mm-phys], [Enables output of physical memory mapping routines (default: off)])
)

AC_ARG_ENABLE([output-mm-virt],
  AC_HELP_STRING([--enable-output-mm-virt], [Enables output of virtual memory mapping routines (default: off)])
)

AC_ARG_ENABLE([output-mm-heap],
  AC_HELP_STRING([--enable-output-mm-heap], [Enables output of kernel heap routines (default: off)])
)

AC_ARG_ENABLE([output-mm-placement],
  AC_HELP_STRING([--enable-output-mm-placement], [Enables output of placememt allocator (default: off)])
)

AC_ARG_ENABLE([output-mailbox],
  AC_HELP_STRING([--enable-output-mailbox], [Enables output of mailbox functions (default: off)])
)

AC_ARG_ENABLE([output-timer],
  AC_HELP_STRING([--enable-output-timer], [Enables output of timer functions (default: off)])
)

AC_ARG_ENABLE([opt],
  AC_HELP_STRING([--enable-opt], [enable code optimization (default: 2)]),
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
  s)
    CFLAGS="${CFLAGS} -Os"
    ;;
  g)
    CFLAGS="${CFLAGS} -Og"
    ;;
  *)
    CFLAGS="${CFLAGS} -O2"
    ;;
  esac],
  [CFLAGS="${CFLAGS} -O2"]
)

# set target and flags
BOLTHUR_SET_HOST

# set normal flags
BOLTHUR_SET_FLAGS

# conftest
AC_LANG_CONFTEST([AC_LANG_SOURCE([[const char hw[] = "Hello, World\r\n";]])])

# checks for programs
AC_PROG_CC
AM_PROG_AS
AC_PROG_RANLIB
AM_PROG_AR
AC_PROG_CXX
BOLTHUR_PROG_OBJCOPY

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_RESTRICT

# Some compiler dependent features.
AC_DEFINE_UNQUOTED([PACKED], [__attribute__((__packed__))], [Keyword for packing structures.])
AC_DEFINE_UNQUOTED([SECTION(x)], [__attribute__((__section__(x)))], [Keyword for section placement.])
AC_DEFINE_UNQUOTED([ALIGNED(x)], [__attribute__((__aligned__(x)))], [Keyword for alignment.])
AC_DEFINE_UNQUOTED([PACKED_ALIGNED(x)], [__attribute__((__packed__, __aligned__(x)))], [Keyword for packing and aligning structures.])
AC_DEFINE_UNQUOTED([SECTION_ALIGNED(x,y)], [__attribute__((__section__(x), __aligned__(y)))], [Keyword for packing and aligning structures.])
AC_DEFINE_UNQUOTED([OPT_NONE], [__attribute__((__optimize__( "O0" )))], [Disable optimization])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/arch/Makefile
  src/arch/arm/Makefile
  src/arch/arm/v6/Makefile
  src/arch/arm/v7/Makefile
  src/arch/arm/v8/Makefile
  src/kernel/Makefile
  src/lib/Makefile
  src/lib/assert/Makefile
  src/lib/avl/Makefile
  src/lib/ssp/Makefile
  src/lib/stdio/Makefile
  src/lib/stdlib/Makefile
  src/lib/string/Makefile
  src/lib/tar/Makefile
  src/lib/ubsan/Makefile
  src/platform/Makefile
  src/platform/rpi/Makefile
])

AC_OUTPUT
