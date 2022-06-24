
AC_PREREQ([2.71])

AC_INIT([bolthur-server], [0.1.0-dev], [https://github.com/bolthur/driver/issues], [bolthur-server], [https://github.com/bolthur/driver])

AC_COPYRIGHT([Copyright (C) 2018 - 2022 bolthur project])

AC_CONFIG_AUX_DIR([_aux/config])
AC_CONFIG_MACRO_DIR([../../build-aux/m4])

AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([boot/main.c])
AM_INIT_AUTOMAKE([subdir-objects -Werror])
AC_CONFIG_HEADERS([config.h])

AC_LANG([C])

AC_ARG_ENABLE([device],
  AS_HELP_STRING(
    [--enable-device],
    [set target device to compile for [possible targets: raspi0_1, raspi2b_r1, raspi3b]]
  ),
  [enable_device=$enableval],
  [AC_MSG_ERROR([Target device missing])]
)

AC_ARG_ENABLE(
  [release],
  AS_HELP_STRING(
    [--enable-release],
    [activate release build [default: off]]
  ),
  [enable_release=yes]
)

AC_ARG_WITH(
  [optimization-level],
  AS_HELP_STRING(
    [--with-optimization-level],
    [compile with specific code optimization level [possible values: 0, 1, 2, 3, s, g | default: 2]]
  ),
  [with_optimization_level=$withval]
)

AC_ARG_WITH(
  [debug-symbols],
  AS_HELP_STRING(
    [--with-debug-symbols],
    [compile with debug symbols]
  ),
  [with_debug_debug_symbols=yes]
)

# enable output and enable remote debugging together is not allowed
AS_IF([test "x$enable_debug" == "xyes" && test "x$enable_release" == "xyes"], [
  AC_MSG_ERROR([Enabled debug and release are not possible at the same time])
])

# check necessary programs
BOLTHUR_PROG_READLINK

# set program
BOLTHUR_LANG_PROGRAM
# set target and flags
BOLTHUR_SET_DEVICE
BOLTHUR_SERVER_SET_HOST
# set rest of flags
BOLTHUR_SERVER_SET_FLAG

# checks for programs
AC_PROG_CC
AM_PROG_AS
BOLTHUR_PROG_OBJCOPY
BOLTHUR_PROG_STRIP

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_RESTRICT

# init libtool
LT_INIT
LT_LANG([C])

# embed git revision and compiler
AC_DEFINE_UNQUOTED(
  [PACKAGE_REVISION],
  ["m4_esyscmd_s([git describe --always])"],
  [Define to the revision of this package.]
)
AC_DEFINE_UNQUOTED(
  [PACKAGE_COMPILER],
  ["$CC"],
  [Define to the used compiler of this package.]
)

# Some compiler dependent features.
AC_DEFINE_UNQUOTED(
  [__packed],
  [__attribute__((__packed__))],
  [Keyword for packing structures.]
)
AC_DEFINE_UNQUOTED(
  [__section(x)],
  [__attribute__((__section__(x)))],
  [Keyword for section placement.]
)
AC_DEFINE_UNQUOTED(
  [__aligned(x)],
  [__attribute__((__aligned__(x)))],
  [Keyword for alignment.]
)
AC_DEFINE_UNQUOTED(
  [__no_optimization],
  [__attribute__((__optimize__("O0")))],
  [Disable optimization]
)
AC_DEFINE_UNQUOTED(
  [__no_sanitize],
  [__attribute__((no_sanitize("undefined")))],
  [disable undefined behaviour sanitization]
)
AC_DEFINE_UNQUOTED(
  [__no_stack_protector],
  [__attribute__((__optimize__("no-stack-protector")))],
  [disable stack protector mechanism]
)
AC_DEFINE_UNQUOTED(
  [__maybe_unused],
  [__attribute__((unused))],
  [Possibly unused]
)
AC_DEFINE_UNQUOTED(
  [__unused],
  [__attribute__((unused))],
  [Unused]
)

# conditional
AM_CONDITIONAL([IS32], [test $executable_format -eq 32])
AM_CONDITIONAL([IS64], [test $executable_format -eq 64])

AC_CONFIG_FILES([
  Makefile
  boot/Makefile
  console/Makefile
  fs/Makefile
  fs/dev/Makefile
  fs/mount/Makefile
  fs/ramdisk/Makefile
  fs/vfs/Makefile
  platform/Makefile
  platform/raspi/Makefile
  platform/raspi/framebuffer/Makefile
  platform/raspi/iomem/Makefile
  platform/raspi/random/Makefile
  platform/raspi/storage/Makefile
  platform/raspi/storage/sd/Makefile
  terminal/Makefile
])
# FIXME: REPLACE WITH SUBMODULE WHEN MOVING TO OWN REPO

AC_OUTPUT
