
AC_DEFUN([BOLTHUR_DRIVER_PROG_STRIP], [
  AC_CHECK_TOOL([BOLTHUR_STRIP], [strip])
  AC_CACHE_CHECK([whether strip generates $host_bfd],
    [ac_cv_strip_supports_host_bfd],
    [if test "$BOLTHUR_STRIP" --info 2>&1 < /dev/null | grep "$host_bfd" > /dev/null; then
      ac_cv_strip_supports_host_bfd=no
    else
      ac_cv_strip_supports_host_bfd=yes
    fi]
  )
  if test "$ac_cv_strip_supports_host_bfd" == "no"; then
    AC_MSG_ERROR([unsupported host BFD])
  fi
])
