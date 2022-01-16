
AC_DEFUN([BOLTHUR_SET_DEVICE], [
  # set device depending on arguments
  case "${enable_device}" in
    raspi2b_r1)
      DEVICE="raspi2b_r1"
      ;;
    raspi3b)
      DEVICE="raspi3b"
      ;;
    raspi0_1)
      DEVICE="raspi0_1"
      ;;
    *)
      AC_MSG_ERROR([Unknown target device])
      ;;
  esac
])
