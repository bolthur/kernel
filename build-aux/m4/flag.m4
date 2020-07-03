
AC_DEFUN([BOLTHUR_KERNEL_SET_FLAG], [
  # Compilation flags
  # stack protector
  AX_APPEND_COMPILE_FLAGS([-fstack-protector-strong -Wstack-protector], [CFLAGS])
  # warnings
  AX_APPEND_COMPILE_FLAGS([-Wall -Wextra -Werror -Wpedantic], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-Wconversion -Wpacked], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-Wpacked-bitfield-compat], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-Wpacked-not-aligned -Wstrict-prototypes], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-Wmissing-prototypes -Wshadow], [CFLAGS])
  # generic
  AX_APPEND_COMPILE_FLAGS([-fno-exceptions -nodefaultlibs -std=c18], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-fomit-frame-pointer], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-ffreestanding -fno-common], [CFLAGS])

  # default include directories
  AX_APPEND_COMPILE_FLAGS([-I$(readlink -f ${srcdir})/include], [CFLAGS])
  AX_APPEND_COMPILE_FLAGS([-I$(readlink -f ${srcdir})/include/lib], [CFLAGS])

  # debug parameter
  AS_IF([test "x$with_debug_symbols" == "xyes"], [
    # debug symbols and undefined behaviour sanitization
    AX_APPEND_COMPILE_FLAGS([-g -fsanitize=undefined], [CFLAGS])
  ])

  # optimization level
  case "${with_opt}" in
    no | 0)
      AX_APPEND_COMPILE_FLAGS([-O0], [CFLAGS])
      ;;
    1)
      AX_APPEND_COMPILE_FLAGS([-O1], [CFLAGS])
      ;;
    2)
      AX_APPEND_COMPILE_FLAGS([-O2], [CFLAGS])
      ;;
    3)
      AX_APPEND_COMPILE_FLAGS([-O3], [CFLAGS])
      ;;
    s)
      AX_APPEND_COMPILE_FLAGS([-Os], [CFLAGS])
      ;;
    g)
      AX_APPEND_COMPILE_FLAGS([-Og], [CFLAGS])
      ;;
    *)
      AX_APPEND_COMPILE_FLAGS([-O2], [CFLAGS])
      ;;
  esac

  # linker flags
  AX_APPEND_LINK_FLAGS([-nostdlib], [LDFLAGS])
  AX_APPEND_LINK_FLAGS([-Wl,-u,Entry], [LDFLAGS])
])
