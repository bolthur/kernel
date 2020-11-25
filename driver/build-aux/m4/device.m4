
AC_DEFUN([BOLTHUR_DRIVER_SET_DEVICE], [
  # set device depending on arguments
  case "${enable_device}" in
    rpi2_b_rev1)
      DEVICE="rpi2_b_rev1"
      ;;
    rpi3_b)
      DEVICE="rpi3_b"
      ;;
    rpi_zero_w)
      DEVICE="rpi_zero_w"
      ;;
    *)
      AC_MSG_ERROR([Unknown target device])
      ;;
  esac
])
