
AC_PREREQ([2.70])

AC_INIT([bolthur-driver], [0.1.0-dev], [https://github.com/bolthur/driver/issues], [bolthur-driver], [https://github.com/bolthur/driver])

AC_COPYRIGHT([Copyright (C) 2018 - 2020 bolthur project])

AC_CONFIG_AUX_DIR([build-aux/config])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([src/generic/init/main.c])
AM_INIT_AUTOMAKE([subdir-objects -Werror])
AC_CONFIG_HEADERS([include/config.h])

AC_LANG([C])

AC_ARG_ENABLE([device],
  AS_HELP_STRING(
    [--enable-device],
    [set target device to compile for [possible targets: rpi2_b_rev1]]
  ),
  [enable_device=$enableval],
  [AC_MSG_ERROR([Target device missing])]
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

# check necessary programs
BOLTHUR_DRIVER_PROG_READLINK

# set program
BOLTHUR_DRIVER_LANG_PROGRAM
# set target and flags
BOLTHUR_DRIVER_SET_DEVICE
BOLTHUR_DRIVER_SET_HOST
# set rest of flags
BOLTHUR_DRIVER_SET_FLAG

# checks for programs
AC_PROG_CC
AM_PROG_AS
BOLTHUR_DRIVER_PROG_OBJCOPY

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_RESTRICT

# init libtool
LT_INIT([disable-shared static])
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

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/generic/Makefile
  src/generic/init/Makefile
  src/generic/vfs/Makefile
  src/platform/Makefile
  src/platform/rpi/Makefile
  src/platform/rpi/framebuffer/Makefile
  thirdparty/Makefile
  thirdparty/dtc/libfdt/Makefile
  thirdparty/font/Makefile
  thirdparty/zlib/Makefile
])
# FIXME: REPLACE WITH SUBMODULE WHEN MOVING TO OWN REPO
AM_CONDITIONAL([ENABLE_ZLIB], [true])

AC_OUTPUT
