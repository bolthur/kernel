
AC_PREREQ(2.57)
AC_INIT([myst-system-kernel], [0.1], [bug-kernel@myst-system.org])

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
  rpi2_1)
    DEVICE="rpi2_1"
    ;;
  rpi2_2)
    DEVICE="rpi2_2"
    ;;
  rpi3)
    DEVICE="rpi3"
    ;;
  *)
    DEVICE="pc"
    ;;
  esac],
  [DEVICE="pc"]
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

AC_LANG_CONFTEST([AC_LANG_SOURCE([[const char hw[] = "Hello, World\n";]])])

# set cc to clang
AC_PROG_CC([clang, gcc])

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
AC_TYPE_SIZE_T
AC_C_RESTRICT

AC_CHECK_FUNC( __aeabi_uidivmod )

# Some compiler dependent features.
AC_DEFINE_UNQUOTED([PACKED], [__attribute__((packed))], [Keyword for packing structures.])
AC_DEFINE_UNQUOTED([SECTION(x)], [__attribute__((section(x)))], [Keyword for section placement.])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/kernel/Makefile
  src/kernel/arch/Makefile
  src/kernel/arch/arm/Makefile
  src/kernel/arch/arm/v7/Makefile
  src/kernel/arch/arm/v8/Makefile
  src/kernel/arch/x86/Makefile
  src/kernel/arch/x86/ia32/Makefile
  src/kernel/arch/x86/amd64/Makefile
  src/kernel/vendor/Makefile
  src/kernel/vendor/pc/Makefile
  src/kernel/vendor/rpi/Makefile
  src/libc/Makefile
])

AC_OUTPUT
