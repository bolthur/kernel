
AC_DEFUN([BOLTHUR_KERNEL_PREPARE_CPPCHECK], [
  dir=$($BOLTHUR_READLINK -f ${srcdir})/build-aux/platform/${platform_subdir}
  AC_SUBST(CPPCHECK_PLATFORM, ${dir}/cppcheck.${executable_format}.xml)
  AC_SUBST(CPPCHECK_INCLUDE, ${ac_pwd}/include/config.h)
  AC_SUBST(CPPCHECK_PROJECT, ${dir}/project.${executable_format}.cppcheck)
  AC_SUBST(CPPCHECK_SUPPRESS, ${dir}/suppress.${executable_format}.txt)
])
