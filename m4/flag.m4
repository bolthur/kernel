
AC_DEFUN([BOLTHUR_SET_FLAGS], [
  CFLAGS="${CFLAGS} -ffreestanding -fno-exceptions -Wall -Wextra -Werror -Wpedantic -Wconversion -Wpacked -Wpacked-bitfield-compat -Wpacked-not-aligned -nodefaultlibs -std=c18"
  LDFLAGS="${LDFLAGS} -nostdlib"
])
