
AC_DEFUN([BOLTHUR_KERNEL_PROG_OBJCOPY], [
  AC_CHECK_TOOL([BOLTHUR_OBJCOPY], [objcopy])
  AC_CACHE_CHECK([whether objcopy generates $host_bfd],
    [ac_cv_objcopy_supports_host_bfd],
    [if test "$BOLTHUR_OBJCOPY" --info 2>&1 < /dev/null | grep "$host_bfd" > /dev/null; then
      ac_cv_objcopy_supports_host_bfd=no
    else
      ac_cv_objcopy_supports_host_bfd=yes
    fi]
  )
  if test "$ac_cv_objcopy_supports_host_bfd" == "no"; then
    AC_MSG_ERROR([unsupported host BFD])
  fi
])
