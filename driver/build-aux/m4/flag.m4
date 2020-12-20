
AC_DEFUN([BOLTHUR_DRIVER_SET_FLAG], [
  # Compilation flags
  # FIXME: Add stack protector if working
  # AX_APPEND_COMPILE_FLAGS([-fstack-protector-all -Wstack-protector])
  # warnings
  AX_APPEND_COMPILE_FLAGS([-Wall -Wextra -Werror -Wpedantic])
  AX_APPEND_COMPILE_FLAGS([-Wconversion -Wpacked -Wredundant-decls])
  AX_APPEND_COMPILE_FLAGS([-Wmisleading-indentation -Wundef])
  AX_APPEND_COMPILE_FLAGS([-Wpacked-bitfield-compat -Wrestrict])
  AX_APPEND_COMPILE_FLAGS([-Wpacked-not-aligned -Wstrict-prototypes])
  AX_APPEND_COMPILE_FLAGS([-Wmissing-prototypes -Wshadow])
  AX_APPEND_COMPILE_FLAGS([-Wmissing-noreturn -Wmissing-format-attribute])
  AX_APPEND_COMPILE_FLAGS([-Wduplicated-branches -Wduplicated-cond])
  # generic
  AX_APPEND_COMPILE_FLAGS([-fno-exceptions -std=c18])
  AX_APPEND_COMPILE_FLAGS([-fomit-frame-pointer])

  # debug parameter
  AS_IF([test "x$with_debug_symbols" == "xyes"], [
    # debug symbols and sanitizer
    # -fsanitize=undefined
    AX_APPEND_COMPILE_FLAGS([-g])
  ])

  # optimization level
  case "${with_optimization_level}" in
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
])