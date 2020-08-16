
AC_DEFUN([BOLTHUR_KERNEL_PROG_READLINK], [
  # check for tools to exist
  AC_CHECK_TOOLS([BOLTHUR_READLINK],[greadlink readlink],[no])
  # handle missing
  if test "$BOLTHUR_READLINK" == "no"; then
    AC_MSG_ERROR([readlink not found])
  fi
  # check for parameter -f
  AC_CACHE_CHECK([whether readlink supports -f],
    [ac_cv_readlink_supports_arg],
    [if test "BOLTHUR_READLINK" -f 2>&1 < /dev/null | grep "missing operand" > /dev/null; then
      ac_cv_readlink_supports_arg=no
    else
      ac_cv_readlink_supports_arg=yes
    fi]
  )
  # evaluate result
  if test "$ac_cv_readlink_supports_arg" == "no"; then
    AC_MSG_ERROR([readlink does not support necessary parameter -f])
  fi
])
