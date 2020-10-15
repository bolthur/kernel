
AC_DEFUN([BOLTHUR_KERNEL_SET_FLAG], [
  # Linker flags ( first due to stack protector and no standard library )
  AX_APPEND_LINK_FLAGS([-nostdlib])
  AX_APPEND_LINK_FLAGS([-ffreestanding])
  AX_APPEND_LINK_FLAGS([-Wl,-u,Entry])

  # Compilation flags
  # stack protector
  AX_APPEND_COMPILE_FLAGS([-fstack-protector-all -Wstack-protector])
  # warnings
  AX_APPEND_COMPILE_FLAGS([-Wall -Wextra -Werror -Wpedantic])
  AX_APPEND_COMPILE_FLAGS([-Wconversion -Wpacked])
  AX_APPEND_COMPILE_FLAGS([-Wpacked-bitfield-compat])
  AX_APPEND_COMPILE_FLAGS([-Wpacked-not-aligned -Wstrict-prototypes])
  AX_APPEND_COMPILE_FLAGS([-Wmissing-prototypes -Wshadow])
  # generic
  AX_APPEND_COMPILE_FLAGS([-fno-exceptions -nodefaultlibs -std=c18])
  AX_APPEND_COMPILE_FLAGS([-fomit-frame-pointer])
  AX_APPEND_COMPILE_FLAGS([-ffreestanding -fno-common])

  # debug parameter
  AS_IF([test "x$with_debug_symbols" == "xyes"], [
    # debug symbols and undefined behaviour sanitization
    AX_APPEND_COMPILE_FLAGS([-g -fsanitize=undefined])
  ])

  # optimization level
  case "${with_opt}" in
    no | 0)
      AX_APPEND_COMPILE_FLAGS([-O0])
      ;;
    1)
      AX_APPEND_COMPILE_FLAGS([-O1])
      ;;
    2)
      AX_APPEND_COMPILE_FLAGS([-O2])
      ;;
    3)
      AX_APPEND_COMPILE_FLAGS([-O3])
      ;;
    s)
      AX_APPEND_COMPILE_FLAGS([-Os])
      ;;
    g)
      AX_APPEND_COMPILE_FLAGS([-Og])
      ;;
    *)
      AX_APPEND_COMPILE_FLAGS([-O2])
      ;;
  esac

  # default include directories
  AX_APPEND_COMPILE_FLAGS([-I${ac_pwd}/include])
  AX_APPEND_COMPILE_FLAGS([-I$($BOLTHUR_READLINK -f ${srcdir})/include])
  AX_APPEND_COMPILE_FLAGS([-I$($BOLTHUR_READLINK -f ${srcdir})/include/lib])
  AX_APPEND_COMPILE_FLAGS([-I$($BOLTHUR_READLINK -f ${srcdir})/thirdparty])
  AX_APPEND_COMPILE_FLAGS([-I$($BOLTHUR_READLINK -f ${srcdir})/thirdparty/dtc/libfdt])
  AX_APPEND_COMPILE_FLAGS([-imacros\ $($BOLTHUR_READLINK -f ${srcdir})/include/core/config.h])

  # Populate libraries for linker
  CC_SPECIFIC_LDADD=;
  if `$CC -v 2>&1 | grep 'gcc version' >/dev/null 2>&1` ; then
    CC_SPECIFIC_LDADD="-lgcc"
  fi
  AC_SUBST(CC_SPECIFIC_LDADD, $CC_SPECIFIC_LDADD)
])
