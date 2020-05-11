
AC_PREREQ(2.69)
AC_INIT([bolthur-kernel], [0.1], [https://github.com/bolthur/kernel/issues], [bolthur-kernel], [https://github.com/bolthur/kernel])
AC_COPYRIGHT([Copyright (C) 2018 - 2020 bolthur project])

AC_CONFIG_AUX_DIR([build-aux/config])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_CANONICAL_HOST
AC_CONFIG_SRCDIR([src/core/main.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([include/config.h])

AC_LANG([C])

# Prevent enter folder output
AC_SUBST(MAKEFLAGS, "${MAKEFLAGS} --no-print-directory")

AC_ARG_ENABLE([device],
  AC_HELP_STRING(
    [--enable-device],
    [set target device to compile for [possible targets: rpi2_b_rev1]]
  ),
  [enable_device=$enableval],
  [AC_MSG_ERROR([Target device missing])]
)

AC_ARG_ENABLE(
  [output],
  AC_HELP_STRING(
    [--enable-output],
    [activate kernel output [default: off]]
  ),
  [enable_output=yes]
)

AC_ARG_ENABLE(
  [output-mm-phys],
  AC_HELP_STRING(
    [--enable-output-mm-phys],
    [activate physical memory mapping output [default: off]]
  ),
  [enable_output_mm_phys=yes]
)

AC_ARG_ENABLE(
  [output-mm-virt],
  AC_HELP_STRING(
    [--enable-output-mm-virt],
    [activate virtual memory mapping output [default: off]]
  ),
  [enable_output_mm_virt=yes]
)

AC_ARG_ENABLE(
  [output-mm-heap],
  AC_HELP_STRING(
    [--enable-output-mm-heap],
    [activate heap output [default: off]]
  ),
  [enable_output_mm_heap=yes]
)

AC_ARG_ENABLE(
  [output-mailbox],
  AC_HELP_STRING(
    [--enable-output-mailbox],
    [activate mailbox output [default: off]]
  ),
  [enable_output_mailbox=yes]
)

AC_ARG_ENABLE(
  [output-timer],
  AC_HELP_STRING(
    [--enable-output-timer],
    [activate timer output [default: off]]
  ),
  [enable_output_timer=yes]
)

AC_ARG_ENABLE(
  [output-initrd],
  AC_HELP_STRING(
    [--enable-output-initrd],
    [activate initrd output [default: off]]
  ),
  [enable_output_initrd=yes]
)

AC_ARG_ENABLE(
  [output-event],
  AC_HELP_STRING(
    [--enable-output-event],
    [activate event output [default: off]]
  ),
  [enable_output_event=yes]
)

AC_ARG_ENABLE(
  [output-process],
  AC_HELP_STRING(
    [--enable-output-process],
    [activate task output [default: off]]
  ),
  [enable_output_process=yes]
)

AC_ARG_ENABLE(
  [output-interrupt],
  AC_HELP_STRING(
    [--enable-output-interrupt],
    [activate interrupt output [default: off]]
  ),
  [enable_output_interrupt=yes]
)

AC_ARG_ENABLE(
  [output-exception],
  AC_HELP_STRING(
    [--enable-output-exception],
    [activate exception output [default: off]]
  ),
  [enable_output_exception=yes]
)

AC_ARG_ENABLE(
  [output-elf],
  AC_HELP_STRING(
    [--enable-output-elf],
    [activate elf output [default: off]]
  ),
  [enable_output_elf=yes]
)

AC_ARG_ENABLE(
  [output-platform],
  AC_HELP_STRING(
    [--enable-output-platform],
    [activate platform init output [default: off]]
  ),
  [enable_output_platform=yes]
)

AC_ARG_ENABLE(
  [output-syscall],
  AC_HELP_STRING(
    [--enable-output-syscall],
    [activate syscall output [default: off]]
  ),
  [enable_output_syscall=yes]
)

AC_ARG_ENABLE(
  [output-serial],
  AC_HELP_STRING(
    [--enable-output-serial],
    [activate serial handling output [default: off]]
  ),
  [enable_output_serial=yes]
)

AC_ARG_ENABLE(
  [remote-debug],
  AC_HELP_STRING(
    [--enable-remote-debug],
    [activate remote debugging [default: off]]
  ),
  [enable_remote_debug=yes]
)

AC_ARG_WITH(
  [optimization-level],
  AC_HELP_STRING(
    [--with-optimization-level],
    [compile with specific code optimization level [possible values: 0, 1, 2, 3, s, g | default: 2]]
  ),
  [with_optimization_level=$withval]
)

AC_ARG_WITH(
  [debug-symbols],
  AC_HELP_STRING(
    [--with-debug-symbols],
    [compile with debug symbols]
  ),
  [with_debug_debug_symbols=yes]
)

# set target and flags
BOLTHUR_KERNEL_SET_DEVICE
BOLTHUR_KERNEL_SET_HOST
# set rest of flags
BOLTHUR_KERNEL_SET_FLAG
BOLTHUR_KERNEL_LANG_PROGRAM

# checks for programs
AC_PROG_CC
AM_PROG_AS
AC_PROG_RANLIB
AM_PROG_AR
BOLTHUR_KERNEL_PROG_OBJCOPY

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_RESTRICT

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
  [__bootstrap],
  [__section(".text.boot")],
  [Bootstrap function]
)
AC_DEFINE_UNQUOTED(
  [__bootstrap_data],
  [__section(".data.boot")],
  [Bootstrap data]
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
AC_DEFINE_UNQUOTED(
  [__no_sanitize],
  [__attribute__((no_sanitize("undefined")))],
  [disable undefined behaviour sanitization]
)
AC_DEFINE_UNQUOTED(
  [__no_return],
  [__attribute__((noreturn))],
  [mark function as function without continuing exectution]
)
AC_DEFINE_UNQUOTED(
  [__constructor],
  [__attribute__((constructor))],
  [mark function as constructor]
)

# conditional
AM_CONDITIONAL([IS32], [test $executable_format -eq 32])
AM_CONDITIONAL([IS64], [test $executable_format -eq 64])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/arch/Makefile
  src/arch/arm/Makefile
  src/arch/arm/v6/Makefile
  src/arch/arm/v7/Makefile
  src/arch/arm/v8/Makefile
  src/core/Makefile
  src/lib/Makefile
  src/lib/assert/Makefile
  src/lib/atag/Makefile
  src/lib/avl/Makefile
  src/lib/list/Makefile
  src/lib/ssp/Makefile
  src/lib/stdio/Makefile
  src/lib/stdlib/Makefile
  src/lib/string/Makefile
  src/lib/tar/Makefile
  src/lib/ubsan/Makefile
  src/platform/Makefile
  src/platform/rpi/Makefile
  src/target/Makefile
  src/target/rpi/Makefile
  thirdparty/Makefile
  thirdparty/font/Makefile
])

AC_OUTPUT
