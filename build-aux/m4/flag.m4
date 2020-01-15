
AC_DEFUN([BOLTHUR_KERNEL_SET_FLAG], [
  # stack protector
  CFLAGS="${CFLAGS} -fstack-protector-strong -Wstack-protector"
  # warnings
  CFLAGS="${CFLAGS} -Wall -Wextra -Werror -Wpedantic -Wconversion -Wpacked"
  CFLAGS="${CFLAGS} -Wpacked-bitfield-compat -Wpacked-not-aligned"
  # sanitize
  # CFLAGS="${CFLAGS} -fsanitize=undefined"
  # generic
  CFLAGS="${CFLAGS} -ffreestanding -fno-exceptions -nodefaultlibs -std=c18"
  CFLAGS="${CFLAGS} -fomit-frame-pointer -ffreestanding"

  # debug parameter
  AS_IF([test "x$with_debug_symbols" == "xyes"], [
    CFLAGS="${CFLAGS} -g3"
  ])
  # optimization level
  case "${with_opt}" in
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
  esac

  # linker flags
  LDFLAGS="${LDFLAGS} -nostdlib"
])
