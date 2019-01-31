
AC_PREREQ(2.57)
AC_INIT([bolthur-kernel], [0.1], [bug-kernel@bolthur.org])
AC_COPYRIGHT([Copyright (C) 2017 - 2019 bolthur project])

AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([kernel/kernel/main.c])
AM_INIT_AUTOMAKE([foreign 1.7])
AC_CONFIG_HEADERS([config.h])

AC_LANG([C])

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

AC_ARG_ENABLE([serial-tty],
  AC_HELP_STRING([--enable-serial-tty], [switch kernel tty output to serial only (default: off)])
)

AC_ARG_ENABLE([output-mm-phys],
  AC_HELP_STRING([--enable-output-mm-phys], [Enables output of physical memory mapping routines (default: off)])
)

AC_ARG_ENABLE([output-mm-virt],
  AC_HELP_STRING([--enable-output-mm-phys], [Enables output of virtual memory mapping routines (default: off)])
)

AC_ARG_ENABLE([output-mm-heap],
  AC_HELP_STRING([--enable-output-mm-phys], [Enables output of kernel heap routines (default: off)])
)

AC_ARG_ENABLE([output-mailbox],
  AC_HELP_STRING([--enable-output-mailbox], [Enables output of mailbox functions (default: off)])
)

AC_ARG_ENABLE([opt],
  AC_HELP_STRING([--disable-opt], [disable code optimization (default: on)]),
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

# set target and flags
BOLTHUR_SET_HOST

# set normal flags
BOLTHUR_SET_FLAGS

# extend path with local toolchain
PATH="${ac_abs_confdir}/scripts/opt/cross/bin:${PATH}"
AC_SUBST(PATH, "${ac_abs_confdir}/scripts/opt/cross/bin:${PATH}")

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
AC_DEFINE_UNQUOTED([PACKED], [__attribute__((packed))], [Keyword for packing structures.])
AC_DEFINE_UNQUOTED([SECTION(x)], [__attribute__((section(x)))], [Keyword for section placement.])

AC_CONFIG_FILES([
  Makefile
  kernel/Makefile
  kernel/kernel/Makefile
  kernel/arch/Makefile
  kernel/arch/arm/Makefile
  kernel/arch/arm/v6/Makefile
  kernel/arch/arm/v7/Makefile
  kernel/arch/arm/v8/Makefile
  kernel/vendor/Makefile
  kernel/vendor/rpi/Makefile
  lib/Makefile
  lib/atag/Makefile
  lib/avl/Makefile
  lib/fdt/Makefile
  lib/stdc/Makefile
  lib/tar/Makefile
])

AC_OUTPUT
