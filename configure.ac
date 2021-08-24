
LT_PREREQ([2.4.6])
AC_PREREQ([2.71])

AC_INIT([bolthur-kernel], [0.1.0-dev], [https://github.com/bolthur/kernel/issues], [bolthur-kernel], [https://github.com/bolthur/kernel])

AC_COPYRIGHT([Copyright (C) 2018 - 2021 bolthur project])

AC_CONFIG_AUX_DIR([build-aux/config])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([nostdinc subdir-objects -Werror])

AC_LANG([C])

AC_ARG_ENABLE([device],
  AS_HELP_STRING(
    [--enable-device],
    [set target device to compile for [possible targets: rpi2_b_rev1]]
  ),
  [enable_device=$enableval],
  [AC_MSG_ERROR([Target device missing])]
)

AC_ARG_ENABLE(
  [output],
  AS_HELP_STRING(
    [--enable-output],
    [activate kernel output [default: off]]
  ),
  [enable_output=yes]
)

AC_ARG_ENABLE(
  [output-mm-phys],
  AS_HELP_STRING(
    [--enable-output-mm-phys],
    [activate physical memory mapping output [default: off]]
  ),
  [enable_output_mm_phys=yes]
)

AC_ARG_ENABLE(
  [output-mm-virt],
  AS_HELP_STRING(
    [--enable-output-mm-virt],
    [activate virtual memory mapping output [default: off]]
  ),
  [enable_output_mm_virt=yes]
)

AC_ARG_ENABLE(
  [output-mm-heap],
  AS_HELP_STRING(
    [--enable-output-mm-heap],
    [activate heap output [default: off]]
  ),
  [enable_output_mm_heap=yes]
)

AC_ARG_ENABLE(
  [output-mm-shared],
  AS_HELP_STRING(
    [--enable-output-mm-shared],
    [activate shared memory output [default: off]]
  ),
  [enable_output_mm_shared=yes]
)

AC_ARG_ENABLE(
  [output-mailbox],
  AS_HELP_STRING(
    [--enable-output-mailbox],
    [activate mailbox output [default: off]]
  ),
  [enable_output_mailbox=yes]
)

AC_ARG_ENABLE(
  [output-timer],
  AS_HELP_STRING(
    [--enable-output-timer],
    [activate timer output [default: off]]
  ),
  [enable_output_timer=yes]
)

AC_ARG_ENABLE(
  [output-initrd],
  AS_HELP_STRING(
    [--enable-output-initrd],
    [activate initrd output [default: off]]
  ),
  [enable_output_initrd=yes]
)

AC_ARG_ENABLE(
  [output-event],
  AS_HELP_STRING(
    [--enable-output-event],
    [activate event output [default: off]]
  ),
  [enable_output_event=yes]
)

AC_ARG_ENABLE(
  [output-process],
  AS_HELP_STRING(
    [--enable-output-process],
    [activate task output [default: off]]
  ),
  [enable_output_process=yes]
)

AC_ARG_ENABLE(
  [output-interrupt],
  AS_HELP_STRING(
    [--enable-output-interrupt],
    [activate interrupt output [default: off]]
  ),
  [enable_output_interrupt=yes]
)

AC_ARG_ENABLE(
  [output-exception],
  AS_HELP_STRING(
    [--enable-output-exception],
    [activate exception output [default: off]]
  ),
  [enable_output_exception=yes]
)

AC_ARG_ENABLE(
  [output-elf],
  AS_HELP_STRING(
    [--enable-output-elf],
    [activate elf output [default: off]]
  ),
  [enable_output_elf=yes]
)

AC_ARG_ENABLE(
  [output-syscall],
  AS_HELP_STRING(
    [--enable-output-syscall],
    [activate syscall output [default: off]]
  ),
  [enable_output_syscall=yes]
)

AC_ARG_ENABLE(
  [output-serial],
  AS_HELP_STRING(
    [--enable-output-serial],
    [activate serial handling output [default: off]]
  ),
  [enable_output_serial=yes]
)

AC_ARG_ENABLE(
  [output-message],
  AS_HELP_STRING(
    [--enable-output-message],
    [activate message output [default: off]]
  ),
  [enable_output_message=yes]
)

AC_ARG_ENABLE(
  [output-rpc],
  AS_HELP_STRING(
    [--enable-output-rpc],
    [activate rpc output [default: off]]
  ),
  [enable_output_rpc=yes]
)

AC_ARG_ENABLE(
  [remote-debug],
  AS_HELP_STRING(
    [--enable-remote-debug],
    [activate remote debugging [default: off]]
  ),
  [enable_remote_debug=yes]
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
AS_IF([test "x$enable_output" == "xyes" && test "x$enable_remote_debug" == "xyes"], [
  AC_MSG_ERROR([Enabled kernel output and remote debug are not possible at the same time])
])

# check necessary programs
BOLTHUR_PROG_READLINK

# set program
BOLTHUR_LANG_PROGRAM
# set target and flags
BOLTHUR_SET_DEVICE
BOLTHUR_KERNEL_SET_HOST

# checks for programs
AC_PROG_CC
AM_PROG_AS
BOLTHUR_PROG_OBJCOPY
BOLTHUR_PROG_STRIP

# prepare cppecheck stuff
BOLTHUR_PREPARE_CPPCHECK
# add directory for ramdisk
AC_SUBST(ROOT_DIR, $($BOLTHUR_READLINK -f ${srcdir}))
AC_SUBST(SYSROOT_DIR, $(dirname $(dirname $($BOLTHUR_READLINK -f $($CC $CFLAGS --print-file-name=libc.so)))))

# doxygen stuff
DX_DOXYGEN_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_MAN_FEATURE(ON)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([bolthur/kernel], [$(srcdir)/doxyfile], [doc])

# checks for programs
AC_PROG_CC
AM_PROG_AS
BOLTHUR_PROG_OBJCOPY
BOLTHUR_PROG_STRIP

# get host cpu count
AX_COUNT_CPUS
AC_SUBST(CPU_COUNT)

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
  [__allocator],
  [__attribute__((malloc))],
  [Keyword for mark something as allocator.]
)
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
  [__no_tree_loop_distribute],
  [__attribute__((__optimize__("no-tree-loop-distribute-patterns")))],
  [disable stuff like replace logic with memset]
)
AC_DEFINE_UNQUOTED(
  [__bootstrap],
  [__section(".text.boot") __no_sanitize __no_stack_protector __no_tree_loop_distribute],
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

AC_CONFIG_FILES([
  Makefile
  bolthur/Makefile
])

AC_CONFIG_SUBDIRS([
  driver
  bolthur/library
  bolthur/kernel
  bolthur/server
])

AC_OUTPUT
