
AC_DEFUN([BOLTHUR_PREPARE_CPPCHECK], [
  dir=$($BOLTHUR_READLINK -f ${srcdir})/build-aux/platform/${platform_subdir}
  AC_SUBST(CPPCHECK_PLATFORM, ${dir}/cppcheck.${executable_format}.xml)
  AC_SUBST(CPPCHECK_KERNEL_INCLUDE, ${ac_pwd}/bolthur/kernel/config.h)
  AC_SUBST(CPPCHECK_KERNEL_PROJECT, ${dir}/project.${executable_format}.kernel.cppcheck)
  AC_SUBST(CPPCHECK_LIBRARY_INCLUDE, ${ac_pwd}/bolthur/library/config.h)
  AC_SUBST(CPPCHECK_LIBRARY_PROJECT, ${dir}/project.${executable_format}.library.cppcheck)
  AC_SUBST(CPPCHECK_SERVER_INCLUDE, ${ac_pwd}/bolthur/server/config.h)
  AC_SUBST(CPPCHECK_SERVER_PROJECT, ${dir}/project.${executable_format}.server.cppcheck)
  AC_SUBST(CPPCHECK_APPLICATION_INCLUDE, ${ac_pwd}/bolthur/application/config.h)
  AC_SUBST(CPPCHECK_APPLICATION_PROJECT, ${dir}/project.${executable_format}.application.cppcheck)
  AC_SUBST(CPPCHECK_SUPPRESS, ${dir}/suppress.${executable_format}.txt)
])
