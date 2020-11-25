
AC_DEFUN([BOLTHUR_DRIVER_SET_HOST], [
  case "${host_cpu}" in
  arm)
    arch_subdir=arm
    host_bfd=elf32-littlearm
    CFLAGS="${CFLAGS} -marm"

    case "${DEVICE}" in
    rpi2_b_rev1)
      CFLAGS="${CFLAGS} -march=armv7-a -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard"
      subarch_subdir=v7
      platform_subdir=rpi
      ;;
    rpi_zero_w)
      CFLAGS="${CFLAGS} -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfpv2 -mfloat-abi=hard"
      subarch_subdir=v6
      platform_subdir=rpi
      ;;
    rpi3_b)
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53 -mfpu=neon-vfpv4 -mfloat-abi=hard"
      subarch_subdir=v8
      platform_subdir=rpi
      ;;
    *)
      AC_MSG_ERROR([unsupported host platform])
      ;;
    esac
    ;;
  aarch64)
    arch_subdir=arm
    host_bfd=elf64-littleaarch64

    case "${DEVICE}" in
    rpi3_b)
      CFLAGS="${CFLAGS} -march=armv8-a -mtune=cortex-a53"
      subarch_subdir=v8
      platform_subdir=rpi
      ;;
    *)
      AC_MSG_ERROR([unsupported host platform])
      ;;
    esac
    ;;
  *)
    AC_MSG_ERROR([unsupported host CPU])
    ;;
  esac

  # copy flags for binary creation
  copy_flags="-I ${host_bfd} -O ${host_bfd}"
  AC_DEFINE_UNQUOTED([ARCH], [${arch_subdir}], [bolthur/kernel target architecture])
  AC_DEFINE_UNQUOTED([SUBARCH], [${subarch_subdir}], [bolthur/kernel target subarchitecture])
  AC_DEFINE_UNQUOTED([PLATFORM], [${platform_subdir}], [bolthur/kernel target platform])
  AC_DEFINE_UNQUOTED(
    [PACKAGE_ARCHITECTURE],
    ["$arch_subdir$subarch_subdir"],
    [Define to the target architecture for this package]
  )
  AC_SUBST(arch_subdir)
  AC_SUBST(subarch_subdir)
  AC_SUBST(platform_subdir)
  AC_SUBST(host_bfd)
  AC_SUBST(copy_flags)
])
