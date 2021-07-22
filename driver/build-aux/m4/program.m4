
AC_DEFUN([BOLTHUR_DRIVER_LANG_PROGRAM], [
  # overwrite c test program
  m4_define([AC_LANG_PROGRAM(C)], [ $1
    int main ( void ) {
      $2
      ;
      return 0;
    }
  ])
])
